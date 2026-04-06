# Chapter 45: Subclassing C++ in Perl (Virtual Overrides)

C++ achieve polymorphism via **Virtual Tables (vtables)**. If a class has a virtual method, the compiler inserts a pointer to a table of function addresses. When you call that method, the CPU looks up the address in the table and jumps to it.

While you cannot easily "hook" into an existing C++ binary's vtable, you can use Affix to build a **Proxy Class** in C++ that accepts Perl callbacks. This allows a C++ engine to call into Perl as if it were calling a native subclass.

## The Recipe

We will create a C++ "Engine" that expects an object with a virtual `on_update` method. We will implement this object in Perl.

```perl
use v5.40;
use Affix qw[:all];
use Affix::Build;
$|++;

# Build the C++ Proxy and Engine
my $c = Affix::Build->new();
$c->add( \<<~'END', lang => 'cpp' );
    #include <iostream>

    // The Base Class (The Interface)
    class Base {
    public:
        virtual ~Base() {}
        virtual void on_update(int value) = 0;
    };

    // The Proxy Class
    // This is a native C++ class that delegates its virtual
    // method to a function pointer (which will be a Perl callback).
    class Proxy : public Base {
    public:
        typedef void (*update_cb)(int);
        update_cb _perl_fn;

        Proxy(update_cb fn) : _perl_fn(fn) {}

        void on_update(int value) override {
            if (_perl_fn) _perl_fn(value);
        }
    };

    // The Engine
    // Simulates a system that works with Base objects
    extern "C" {
        void run_engine(Base* obj, int iterations) {
            for (int i = 0; i < iterations; ++i) {
                obj->on_update(i);
            }
        }

        Base* create_proxy(Proxy::update_cb fn) {
            return new Proxy(fn);
        }

        void destroy_proxy(Base* obj) {
            delete obj;
        }
    }
END
my $lib = $c->link;

# Bind the C++ functions
# Note: Use Pointer[Void] for the C++ object handle
affix $lib, 'create_proxy',  [ Callback [ [Int] => Void ] ] => Pointer [Void];
affix $lib, 'destroy_proxy', [ Pointer [Void] ]             => Void;
affix $lib, 'run_engine',    [ Pointer [Void], Int ]        => Void;

# Implement the Subclass Logic in Perl
my $perl_sub = sub ($val) {
    say 'Perl: Received update from C++ engine: ' . $val;
};

# Use it
say 'Main: Creating C++ Proxy...';
my $obj = create_proxy($perl_sub);
say 'Main: Running Engine...';
run_engine( $obj, 5 );
say 'Main: Cleaning up...';
destroy_proxy($obj);
```

## How It Works

*   **1. The Proxy Pattern**
    Since Perl isn't a C++ compiler, it can't generate a binary vtable that matches the C++ engine's expectations. The `Proxy` class acts as a translator. It is a real C++ object that inherits from the `Base` class, so it has a valid vtable.

*   **2. The Callback Bridge**
    The `Proxy` class stores a function pointer (`_perl_fn`). When the engine calls `obj->on_update()`, the C++ code execution flows into the proxy, which then calls the function pointer.

*   **3. Affix Callbacks**
    When you pass `$perl_sub` to `create_proxy`, Affix generates a reverse-JIT trampoline. This trampoline is a valid C-style function pointer that can be safely called by the C++ code.

*   **4. `extern "C"`**
    C++ mangles function names (see Chapter 1). We wrap our entry points in `extern "C"` to ensure Affix can find the symbols `run_engine` and `create_proxy` using their simple names.

## Kitchen Reminders

*   **Memory Management**
    In this recipe, we manually call `destroy_proxy`. In a real application, you should use `attach_destructor` (see Chapter 20) to ensure that if the Perl variable holding the object pointer is cleared, the C++ `delete` is called automatically.

*   **Stateful Proxies**
    If your Perl "class" needs to maintain state (instance variables), your callback should probably be a method call. You can pass a second `Pointer[Void]` to the C++ proxy to hold a "user data" pointer (which could be an Affix `SV` pointer to your Perl object).

*   **Performance**
    Crossing from C++ to Perl is significantly slower than a native C++ virtual call. If your `on_update` is called 60 times a second (e.g. in a game engine), it's fine. If it's called 60 million times a second, you should implement the logic in C++ instead.
