#include "lib/clutter.h"

#define META_ATTR "ATTR_SUB"

#define MY_CXT_KEY "Dyn::_guts" XS_VERSION

typedef struct
{
    int count;
    HV *classes;
} my_cxt_t;

START_MY_CXT



#define lex_consume(s)  MY_lex_consume(aTHX_ s)
static int MY_lex_consume(pTHX_ char *s)
{    warn("=== %s", __FUNCTION__);

  /* I want strprefix() */
  size_t i;
  for(i = 0; s[i]; i++) {
    if(s[i] != PL_parser->bufptr[i])
      return 0;
  }

  lex_read_to(PL_parser->bufptr + i);
  return i;
}


#define sv_cat_c(SV, U32)  MY_sv_cat_c(aTHX_ SV, U32)
static void MY_sv_cat_c(pTHX_ SV *sv, U32 c)
{    warn("=== %s", __FUNCTION__);

  char ds[UTF8_MAXBYTES + 1], *d;
  d = (char *)uvchr_to_utf8((U8 *)ds, c);
  if (d - ds > 1) {
    sv_utf8_upgrade(sv);
  }
  sv_catpvn(sv, ds, d - ds);
}


#define MY_UNI_IDFIRST(C) isIDFIRST_uni(C)
#define MY_UNI_IDCONT(C)  isALNUM_uni(C)
#if PERL_VERSION_LE(5, 25, 9)
#define MY_UNI_IDFIRST_utf8(P, Z) isIDFIRST_utf8_safe((const unsigned char *)(P), (const unsigned char *)(Z))
#define MY_UNI_IDCONT_utf8(P, Z)  isWORDCHAR_utf8_safe((const unsigned char *)(P), (const unsigned char *)(Z))
#else
#define MY_UNI_IDFIRST_utf8(P, Z) isIDFIRST_utf8((const unsigned char *)(P))
#define MY_UNI_IDCONT_utf8(P, Z)  isALNUM_utf8((const unsigned char *)(P))
#endif


#define lex_scan_ident(bool)  MY_lex_scan_ident(aTHX_ bool)
static SV *MY_lex_scan_ident(pTHX_ bool allow_package)
{
  /* Inspired by
   *   https://metacpan.org/release/MAUKE/Function-Parameters-2.001003/source/Parameters.xs#L351
   */
    warn("=== %s", __FUNCTION__);

    I32 c;
    bool at_start, at_substart;
    SV *ret = newSVpvs("");
    if(lex_bufutf8())
        SvUTF8_on(ret);

    at_start = at_substart = TRUE;

    c = lex_peek_unichar(0);

    while (c != -1) {
        if (at_substart ? MY_UNI_IDFIRST(c) : MY_UNI_IDCONT(c)) {
            lex_read_unichar(0);
            sv_cat_c(ret, c);
            at_substart = FALSE;
            c = lex_peek_unichar(0);
        } else if (allow_package && !at_substart && c == '\'') {
            lex_read_unichar(0);
            c = lex_peek_unichar(0);
            if (!MY_UNI_IDFIRST(c)) {
                lex_stuff_pvs("'", 0);
                break;
            }
            sv_catpvs(ret, "'");
            at_substart = TRUE;
        } else if (allow_package && (at_start || !at_substart) && c == ':') {
            lex_read_unichar(0);
            if (lex_peek_unichar(0) != ':') {
                lex_stuff_pvs(":", 0);
                break;
            }
            lex_read_unichar(0);
            c = lex_peek_unichar(0);
            if (!MY_UNI_IDFIRST(c)) {
                lex_stuff_pvs("::", 0);
                break;
            }
            sv_catpvs(ret, "::");
            at_substart = TRUE;
        } else {
            break;
        }
        at_start = FALSE;
    }

    if(SvCUR(ret))
        return ret;

    SvREFCNT_dec(ret);

    return NULL;
}

#define lex_scan_attr()  MY_lex_scan_attr(aTHX)
static SV *MY_lex_scan_attr(pTHX)
{    warn("=== %s", __FUNCTION__);

  I32 cr = lex_peek_unichar(0);

        warn("Test [%c]", (char)cr);

  lex_read_space(0);SV *ret;

    switch(lex_peek_unichar(0)) {
/*
        case '(':{
            ret = lex_scan_ident(0);
            if(!ret)
                return ret;
            sv_cat_c(ret, lex_read_unichar(0));

            int count = 1;
            I32 c = lex_peek_unichar(0);
            while(count && c != -1) {
                if(c == '(')
                    count++;
                if(c == ')')
                    count--;
                if(c == '\\') {
      /* The next char does not bump count even if it is ( or );
       * the \\ is still captured
       * /
                    sv_cat_c(ret, lex_read_unichar(0));
                    c = lex_peek_unichar(0);
                    if(c == -1)
                        goto unterminated;
                }

                sv_cat_c(ret, lex_read_unichar(0));
                c = lex_peek_unichar(0);
            }

            if(c != -1)
            return ret;
        }
        break;*/
        case ':':
            warn("Yeah!");
            lex_read_space(0); // get rid of it
            lex_read_unichar(0);// get rid of :
            lex_read_space(0); // get rid of it

            ret = lex_scan_ident(1);

            if(!ret)
                return ret;
            else if(memEQ("packed", (const char *)SvPV_nolen(ret), 6)){
                warn("attribute: %s", (const char *)SvPV_nolen(ret));
                return
                ret;
                }
            else
                croak("Unknown struct attribute: %s", (const char *)SvPV_nolen(ret));
            break;

      default:
        return ret;
    }
unterminated:
    croak("Unterminated attribute parameter in attribute list");
}

#define lex_scan_lexvar()  MY_lex_scan_lexvar(aTHX)
static SV *MY_lex_scan_lexvar(pTHX)
{    warn("=== %s", __FUNCTION__);

  int sigil = lex_peek_unichar(0);
  switch(sigil) {
    case '$':
    case '@':
    case '%':
      lex_read_unichar(0);
      break;

    default:
      croak("Expected a lexical variable");
  }

  SV *ret = lex_scan_ident(0);
  if(!ret)
    return NULL;

  /* prepend sigil - which we know to be a single byte */
  SvGROW(ret, SvCUR(ret) + 1);
  Move(SvPVX(ret), SvPVX(ret) + 1, SvCUR(ret), char);
  SvPVX(ret)[0] = sigil;
  SvCUR(ret)++;

  SvPVX(ret)[SvCUR(ret)] = 0;

  return ret;
}

#define lex_scan_parenthesized()  MY_lex_scan_parenthesized(aTHX)
static SV *MY_lex_scan_parenthesized(pTHX)
{
    warn("=== %s", __FUNCTION__);
  I32 c;
  int parencount = 0;
  SV *ret = newSVpvs("");
  if(lex_bufutf8())
    SvUTF8_on(ret);

  c = lex_peek_unichar(0);

  while(c != -1) {
    sv_cat_c(ret, lex_read_unichar(0));

    switch(c) {
      case '(': parencount++; break;
      case ')': parencount--; break;
    }
    if(!parencount)
      break;

    c = lex_peek_unichar(0);
  }

  if(SvCUR(ret))
    return ret;

  SvREFCNT_dec(ret);
  return NULL;
}

#define parse_lexvar()  MY_parse_lexvar(aTHX)
static PADOFFSET MY_parse_lexvar(pTHX)
{    warn("=== %s", __FUNCTION__);

  /* TODO: Rewrite this in terms of using lex_scan_lexvar()
  */
  char *lexname = PL_parser->bufptr;

  if(lex_read_unichar(0) != '$')
    croak("Expected a lexical scalar at %s", lexname);

  if(!isIDFIRST_uni(lex_peek_unichar(0)))
    croak("Expected a lexical scalar at %s", lexname);
  lex_read_unichar(0);
  while(isIDCONT_uni(lex_peek_unichar(0)))
    lex_read_unichar(0);

  /* Forbid $_ */
  if(PL_parser->bufptr - lexname == 2 && lexname[1] == '_')
    croak("Can't use global $_ in \"my\"");

  return pad_add_name_pvn(lexname, PL_parser->bufptr - lexname, 0, NULL, NULL);
}



#define active_keywords_hv()  MY_active_keywords_hv(aTHX)
static HV *MY_active_keywords_hv(pTHX)
{
/* TODO: #ifdef MULTIPLICITY and look in PL_modglobal.
 */
    //warn("=== %s", __FUNCTION__);

  static HV *kw = NULL;
  if(!kw)
    kw = newHV();

  return kw;
}

static int (*next_keyword_plugin)(pTHX_ char *, STRLEN, OP **);

static int my_keyword_plugin(pTHX_ char *kw, STRLEN kwlen, OP **op_ptr)
{
    SV *key = newSVpvf("%.*s", kwlen, kw);
    sv_2mortal(key);

    HE *he = hv_fetch_ent(active_keywords_hv(), key, 0, 0);
    if(!he || !HeVAL(he))
        return (*next_keyword_plugin)(aTHX_ kw, kwlen, op_ptr);

    warn("=== %s( '%s', ... )", __FUNCTION__, kw);
    lex_read_space(0);
    if(memEQ("struct", kw, 6)) {
        warn("STRUCT");

        SV * pkg = lex_scan_ident(1);
        SAVEFREESV(pkg);

        warn("package: %s", (const char *)SvPV_nolen(pkg));

  lex_read_space(0);

  SV *parenstring = lex_scan_attr();
        sv_dump(parenstring);

  //lex_scan_parenthesized();
  SAVEFREESV(parenstring);

  lex_read_space(0);
  warn("lex_peek_unichar(0) == %c", lex_peek_unichar(0));

  switch(lex_peek_unichar(0)){
        case '{': warn("block?");break;// block
        case 'h': warn("has?"); break;
        case 'f': warn("field?"  ); break;
  }
/*
  if(lex_peek_unichar(0) != '{')
    croak("Expected a { BLOCK }");
*/

  I32 floor_ix = start_subparse(FALSE, CVf_ANON);
  SAVEFREESV(PL_compcv);
  I32 save_ix = block_start(TRUE);

  OP *body = parse_block(0);

  SvREFCNT_inc(PL_compcv);
  body = block_end(save_ix, body);

  CV *protocv = newATTRSUB(floor_ix, NULL, NULL, NULL, body);
  SAVEFREESV(protocv);

  {
    dSP;

    ENTER;
    SAVETMPS;

    EXTEND(SP, 3);
    PUSHMARK(SP);
    PUSHs(pkg);
    PUSHs(parenstring);
    mPUSHs(newRV_noinc((SV *)cv_clone(protocv)));
    PUTBACK;

    call_sv(HeVAL(he), G_VOID);

    FREETMPS;
    LEAVE;
  }

  *op_ptr = newOP(OP_NULL, 0);
  return KEYWORD_PLUGIN_STMT;

    }
    else if(memEQ("field", kw, 5)) {
        warn("FIELD");
    }

}








typedef struct Call
{
    DLLib *lib;
    const char *lib_name;
    const char *name;
    const char *sym_name;
    char * sig;
    size_t sig_len;
    char ret;
    DCCallVM *cvm;
    void *fptr;
    char *perl_sig;
} Call;

typedef struct Delayed
{
    const char *library;
    const char *library_version;
    const char *package;
    const char *symbol;
    const char *signature;
    const char *name;
    struct Delayed *next;
} Delayed;

Delayed *delayed; // Not thread safe

static char * clean(char *str) {
    char *end;
    while (isspace(*str) || *str == '"' || *str == '\'')
        str = str + 1;
    end = str + strlen(str) - 1;
    while (end > str && (isspace(*end) || *end == ')' || *end == '"' || *end == '\''))
        end = end - 1;
    *(end + 1) = '\0';
    return str;
}
enum layout { STRUCT, ARRAY, UNION };


// structure of a stack node
struct sNode {
    char data;
  struct sNode *next;
};


SV * struct2sv(void * ptr, DCaggr * ag) {
    for (int i = 0; i < ag->n_fields; ++i)
        warn("Test [%d]", i);
}


void * sv2struct(SV * sv, DCaggr * ag) {

}

// Function to push an item to stack
void _push(struct sNode **top_ref, char new_data) {
  // allocate node
  struct sNode *new_node = (struct sNode *)safemalloc(sizeof(struct sNode));
new_node->data = new_data;
  if (new_node == NULL) {
    warn("Stack overflow\n");
    Safefree(new_node);
    exit(0);
  }

  // link the old list off the new node
  new_node->next = (*top_ref);

  // move the head to point to the new node
  (*top_ref) = new_node;
}

// Function to pop an item from stack
char _pop(struct sNode **top_ref) {
  char res;
  struct sNode *top;

  // If stack is empty then error
  if (*top_ref == NULL)
    croak("Stack overflow");


    top = *top_ref;
    res = top->data;
    *top_ref = top->next;
    Safefree(top);
    return res;


}

int cleanup(struct sNode **top_ref) {
  /* deref top_ref to get the real head */
  struct sNode *current = *top_ref;
  struct sNode *next;

  while (current != NULL) {
    next = current->next;

    free(current);
    current = next;
  }

  /* deref top_ref to affect the real head back
     in the caller. */
  *top_ref = NULL;
}


// Returns 1 if character1 and character2 are matching left
// and right Brackets
bool isMatchingPair(char character1, char character2) {
  if (character1 == '(' && character2 == ')')
    return 1;
  else if (character1 == '{' && character2 == '}')
    return 1;
  else if (character1 == '[' && character2 == ']')
    return 1;
  else if (character1 == '<' && character2 == '>')
    return 1;
  else
    return 0;
}

int parse_signature(pTHX_ Call * call) {
    warn("parse_signature [%d] %s", call->sig_len, call->sig);
    char * sig_ptr = (char *)safesysmalloc(call->sig_len+1);
    Copy(call->sig, sig_ptr, call->sig_len+1, char);
    Zero(call->sig, call->sig_len, char);
    int sig_len = call->sig_len;
    int sig_pos = 0;

    char ch;
    int i, depth = 0;

    struct sNode *stack = (struct sNode *)safemalloc(sizeof(struct sNode));

    for (i = 0; sig_ptr[i + 1] != '\0'; ++i) {

        switch (sig_ptr[i]) {
        case DC_SIGCHAR_CC_PREFIX:
            ++i;
            switch (sig_ptr[i]) {
            case DC_SIGCHAR_CC_DEFAULT:
                dcMode(call->cvm, DC_CALL_C_DEFAULT);
                break;
            case DC_SIGCHAR_CC_THISCALL:
                dcMode(call->cvm, DC_CALL_C_DEFAULT_THIS);
                break;
            case DC_SIGCHAR_CC_ELLIPSIS:
                dcMode(call->cvm, DC_CALL_C_ELLIPSIS);
                break;
            case DC_SIGCHAR_CC_ELLIPSIS_VARARGS:
                dcMode(call->cvm, DC_CALL_C_ELLIPSIS_VARARGS);
                break;
            case DC_SIGCHAR_CC_CDECL:
                dcMode(call->cvm, DC_CALL_C_X86_CDECL);
                break;
            case DC_SIGCHAR_CC_STDCALL:
                dcMode(call->cvm, DC_CALL_C_X86_WIN32_STD);
                break;
            case DC_SIGCHAR_CC_FASTCALL_MS:
                dcMode(call->cvm, DC_CALL_C_X86_WIN32_FAST_MS);
                break;
            case DC_SIGCHAR_CC_FASTCALL_GNU:
                dcMode(call->cvm, DC_CALL_C_X86_WIN32_FAST_GNU);
                break;
            case DC_SIGCHAR_CC_THISCALL_MS:
                dcMode(call->cvm, DC_CALL_C_X86_WIN32_THIS_MS);
                break;
            case DC_SIGCHAR_CC_THISCALL_GNU:
                dcMode(call->cvm, DC_CALL_C_X86_WIN32_FAST_GNU);
                break;
            case DC_SIGCHAR_CC_ARM_ARM:
                dcMode(call->cvm, DC_CALL_C_ARM_ARM);
                break;
            case DC_SIGCHAR_CC_ARM_THUMB:
                dcMode(call->cvm, DC_CALL_C_ARM_THUMB);
                break;
            case DC_SIGCHAR_CC_SYSCALL:
                dcMode(call->cvm, DC_CALL_SYS_DEFAULT);
                break;
            default:
                warn("Unknown signature character: %c at %s line %d", call->sig[i], __FILE__, __LINE__);
                break;
            };
            break;
        case DC_SIGCHAR_VOID:
        case DC_SIGCHAR_BOOL:
        case DC_SIGCHAR_CHAR:
        case DC_SIGCHAR_UCHAR:
        case DC_SIGCHAR_SHORT:
        case DC_SIGCHAR_USHORT:
        case DC_SIGCHAR_INT:
        case DC_SIGCHAR_UINT:
        case DC_SIGCHAR_LONG:
        case DC_SIGCHAR_ULONG:
        case DC_SIGCHAR_LONGLONG:
        case DC_SIGCHAR_ULONGLONG:
        case DC_SIGCHAR_FLOAT:
        case DC_SIGCHAR_DOUBLE: {
            if (depth == 0) call->perl_sig[sig_pos] = '$';
                        call->sig[sig_pos++] = sig_ptr[i];
}
            break;
        case DC_SIGCHAR_POINTER:
        case DC_SIGCHAR_STRING:
        case DC_SIGCHAR_AGGREGATE:
            if (depth == 0) call->perl_sig[sig_pos] = '$';
            call->sig[sig_pos++] = sig_ptr[i];
            break;
        case '<': // union
            if (depth == 0) call->perl_sig[sig_pos] = '$';
            call->sig[sig_pos++] = sig_ptr[i];
            _push(&stack, sig_ptr[i]);
            depth++;
            break;
        case '{': // struct
            if (depth == 0) call->perl_sig[sig_pos] = '%';
            call->sig[sig_pos++] = sig_ptr[i];
            _push(&stack, sig_ptr[i]);
            depth++;
            break;
        case '[': // array
            if (depth == 0) call->perl_sig[sig_pos] = '@';
            call->sig[sig_pos++] = sig_ptr[i];
            _push(&stack, sig_ptr[i]);
            depth++;
            break;
        case '>':
        case '}':
        case ']':

        // If we see an ending bracket without a pair
      // then return false
      if (stack == NULL// depth == -1
         // Pop the top element from stack, if it is not
      // a pair bracket of character then there is a
      // mismatch.
      // This happens for expressions like {(})
      || (!isMatchingPair(_pop(&stack), sig_ptr[i]))) {

        size_t len = strlen(sig_ptr);
        char buffer[len+1];
                warn("here at %s line %d", __FILE__, __LINE__);

                Copy( sig_ptr+1, buffer, i, char);

        buffer[len+1] = '\0';
        croak("Unmatched %c in signature; marked by <-- HERE in %s<-- HERE%s",
             sig_ptr[i], buffer, sig_ptr + i + 1);
        cleanup(&stack);
        return 0;
      }


            call->sig[sig_pos++] = sig_ptr[i];


                warn("here at %s line %d", __FILE__, __LINE__);





            --depth;
            break;
        case DC_SIGCHAR_ENDARG:
            call->sig_len = sig_pos;
            call->ret = sig_ptr[i+1];
            break;
        case '(': // Start of signature

            break;
        default:
            warn("Unknown signature character: %c at %s line %d", call->sig[i], __FILE__, __LINE__);
            break;
        };
    }
                warn("here at %s line %d", __FILE__, __LINE__);


  int ok = stack != NULL;

  Safefree(stack);

                warn("here at %s line %d", __FILE__, __LINE__);



warn("signature now looks like: %s", call->sig);
    return ok ? sig_pos: -1;
}

void push(pTHX_ Call *call, I32 ax) {
     warn("here at %s line %d", __FILE__, __LINE__);
    const char *sig_ptr = call->sig;
    int sig_len = call->sig_len, pos = 0;
    char ch;
    for (ch = *sig_ptr; pos < sig_len; ch = *++sig_ptr,++pos ) {
        warn("pushing #%d [%c], here at %s line %d", pos, ch, __FILE__, __LINE__);

        switch (ch) {
        case DC_SIGCHAR_VOID:
            // TODO: Should I pass a NULL here?
            break;
        case DC_SIGCHAR_BOOL:
            dcArgBool(call->cvm, SvTRUE(ST(pos)));
            break;
        case DC_SIGCHAR_CHAR:
            dcArgChar(call->cvm, (char)SvIV(ST(pos)));
            break;
        case DC_SIGCHAR_UCHAR:
            dcArgChar(call->cvm, (unsigned char)SvIV(ST(pos)));
            break;
        case DC_SIGCHAR_SHORT:
            dcArgShort(call->cvm, (short)SvIV(ST(pos)));
            break;
        case DC_SIGCHAR_USHORT:
            dcArgShort(call->cvm, (unsigned short)SvUV(ST(pos)));
            break;
        case DC_SIGCHAR_INT:
            dcArgInt(call->cvm, (int)SvIV(ST(pos)));
            break;
        case DC_SIGCHAR_UINT:
            dcArgInt(call->cvm, (unsigned int)SvUV(ST(pos)));
            break;
        case DC_SIGCHAR_LONG:
            dcArgLong(call->cvm, (long)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_ULONG:
            dcArgLong(call->cvm, (unsigned long)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_LONGLONG:
            dcArgLongLong(call->cvm, (long long)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_ULONGLONG:
            dcArgLongLong(call->cvm, (unsigned long long)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_FLOAT:
            dcArgFloat(call->cvm, (float)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_DOUBLE:
            dcArgDouble(call->cvm, (double)SvNV(ST(pos)));
            break;
        case DC_SIGCHAR_POINTER:
            if( SvOK(ST(pos)) ) {
                warn("passing pointer at %s line %d", __FILE__, __LINE__);
                IV tmp = SvIV((SV *)SvRV(ST(pos)));
                intptr_t* arg = INT2PTR(intptr_t*, tmp);
                dcArgPointer(call->cvm, arg);
            }
            else {
                warn("passing NULL pointer at %s line %d", __FILE__, __LINE__);
                dcArgPointer(call->cvm, NULL);
            }
            break;
        case DC_SIGCHAR_STRING:
            dcArgPointer(call->cvm, SvOK(ST(pos)) ? (const char *) SvPVutf8_nolen(ST(pos)) : (const char *) NULL);
            break;
        case DC_SIGCHAR_AGGREGATE: {
            ; // empty statement before decl. [C89 vs. C99]
              /* XXX: dyncall structs/union/array aren't ready yet*/
            void *struct_rep;
            Newxz(struct_rep, 0, int); // ha!
            DCaggr *ag;
            if (sv_derived_from(ST(pos), "Dyn::Call::Aggr")) {
                IV tmp = SvIV((SV *)SvRV(ST(pos)));
                ag = INT2PTR(DCaggr *, tmp);
            }
            else
                croak("expected an aggregate but this is not of type Dyn::Call::Aggr");

            //push_aggr(aTHX_ ax, pos, ag, struct_rep);
            dcArgAggr(call->cvm, ag, struct_rep);

            Safefree(struct_rep);
            break;
        }
        case '{': {
            AV * values;
            STMT_START {
                SV* const xsub_tmp_sv = ST(pos);
                SvGETMAGIC(xsub_tmp_sv);
                if (SvROK(xsub_tmp_sv) && SvTYPE(SvRV(xsub_tmp_sv)) == SVt_PVAV)
                    values = (AV*)SvRV(xsub_tmp_sv);
                else
                    Perl_croak_nocontext("struct values must be passed as an array reference");
            } STMT_END;

            warn("hash character: %c at %s line %d", ch, __FILE__, __LINE__);
            bool called = false;
/*
            if (call->aggregate == NULL) {
                Newxz(call->aggregate, 1, Aggr);
                Newxz(agg, 1, DCaggr);

                Newxz(agg_ptr, 0, char); // ha!
                //struct Aggr * next;
            }
            else called = true;
*/

                DCaggr * agg;
                agg = dcNewAggr(1024, 0);
                dcBeginCallAggr(call->cvm, agg);

                void * agg_ptr= safemalloc(0);

       // DCaggr *s = dcNewAggr(1, sizeof(t));


            size_t offset=0;
            size_t agg_pos =-1;
//*++sig_ptr;
            for (ch = *++sig_ptr; pos < sig_len-1; pos++, ch = *++sig_ptr) {
                                agg_pos++;

                warn("    hash content character: %c at %s line %d", ch, __FILE__, __LINE__);

                switch (ch) {
                case DC_SIGCHAR_VOID:
                    // TODO: Should I pass a NULL here?
                    continue;
                case DC_SIGCHAR_BOOL:
                    dcArgBool(call->cvm, SvTRUE(ST(pos)));
                    continue;
                case DC_SIGCHAR_CHAR:{
                    offset += padding_needed_for( offset, 1 );
                                                    if(!called)

                    dcAggrField(agg, ch, offset, 1);
                    DCfield *field = &agg->fields[agg_pos];
                    warn("size: %d | offset: %d", field->size, offset);
                                                    if(!called)

                    agg_ptr = saferealloc(agg_ptr, offset + field->size);
                    SV * s =av_shift(values);
                    char d = (char) SvIV(s);
                    warn("char == %c [%d]", d, d);
                    memcpy(agg_ptr + offset, &d, field->size);
                    offset+=field->size;
                    }
                    continue;
                case DC_SIGCHAR_UCHAR:
                    dcArgChar(call->cvm, (unsigned char)SvIV(ST(pos)));
                    continue;
                case DC_SIGCHAR_SHORT:{
                    offset += padding_needed_for( offset, 2 );
                                if(!called)

                    dcAggrField(agg, ch, offset, 1);
                    DCfield *field = &agg->fields[agg_pos];
                    warn("size: %d | offset: %d", field->size, offset);
                                if(!called)

                    agg_ptr = saferealloc(agg_ptr, offset + field->size);
                    SV * s = av_shift(values);
                    short d = (short) SvIV(s);
                    memcpy(agg_ptr + offset, &d, field->size);
                    offset+=field->size * 1;
                }
                    continue;
                case DC_SIGCHAR_USHORT:
                    dcArgShort(call->cvm, (unsigned short)SvUV(ST(pos)));
                    continue;
                case DC_SIGCHAR_INT:{
                    offset += padding_needed_for( offset, INTSIZE );
                    dcAggrField(agg, ch, offset, 1);
                    DCfield *field = &agg->fields[agg_pos];
                    warn("*size: %d | offset: %d | to: %d", field->size, offset, offset + field->size);
                    int slot = offset + field->size;
                    agg_ptr = saferealloc(agg_ptr, slot);
                    SV ** s = av_fetch(values, agg_pos, 1);
                    int d = (int) SvIV(*s);
                    agg_ptr = saferealloc(agg_ptr,agg->size + field->size);
                    CopyD(&d, agg_ptr + offset,field->size, void);
                    agg->size+=field->size;
                    offset+=field->size * 1; // TODO: 1 here is array size
                }
                    continue;
                case DC_SIGCHAR_UINT:
                    dcArgInt(call->cvm, (unsigned int)SvUV(ST(pos)));
                    continue;
                case DC_SIGCHAR_LONG:
                    dcArgLong(call->cvm, (long)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_ULONG:
                    dcArgLong(call->cvm, (unsigned long)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_LONGLONG:
                    dcArgLongLong(call->cvm, (long long)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_ULONGLONG:
                    dcArgLongLong(call->cvm, (unsigned long long)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_FLOAT:
                    dcArgFloat(call->cvm, (float)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_DOUBLE:
                    dcArgDouble(call->cvm, (double)SvNV(ST(pos)));
                    continue;
                case DC_SIGCHAR_POINTER:
                    warn("passing pointer at %s line %d", __FILE__, __LINE__);
                    {
                        IV tmp = SvIV((SV *)SvRV(ST(pos)));
                        intptr_t * arg = INT2PTR(intptr_t*, tmp);
                        dcArgPointer(call->cvm, arg);
                    }
                    continue;
                case DC_SIGCHAR_STRING:
                    if( SvOK(ST(pos)))
                        warn("OKAY!!!!!!!!!!!!!!!");
                    else
                        warn("NULL!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
                    dcArgPointer(call->cvm, SvOK(ST(pos)) ? SvPVutf8_nolen(ST(pos)) : (char *) NULL);
                    continue;
                case DC_SIGCHAR_AGGREGATE:
                    warn("gotta go deeper at %s line %d", __FILE__, __LINE__);

                    continue;
                default:
                    warn("no idea what to do here at %s line %d", __FILE__, __LINE__);

                    continue;
                }
                break;
            }
            warn("sizeof(hash_rep) == %d at %s line %d", sizeof(agg_ptr), __FILE__, __LINE__);

            dcCloseAggr(agg);
            dcArgAggr(call->cvm, agg, agg_ptr);
            DumpHex(agg_ptr, agg->size);
            //Safefree(agg);
            //Safefree(agg_ptr);
            break;
        }
        case '<':
            warn("union character: %c at %s line %d", ch, __FILE__, __LINE__);

            break;
        case '[':
            warn("array character: %c at %s line %d", ch, __FILE__, __LINE__);

            break;
        case '(':
        case '_':
            pos--;
        break;
        default:
            warn("unhandled signature character: %c at %s line %d", ch, __FILE__, __LINE__);
            break;
        }
    }

    return;
}

SV *retval(pTHX_ Call *call) {
     warn("Here I am! [%c] at %s line %d", call->ret, __FILE__, __LINE__);
    //  TODO: Also sort out pointers that might be return values?
    switch (call->ret) {
    case DC_SIGCHAR_VOID:
        dcCallVoid(call->cvm, call->fptr);
        return &PL_sv_undef;
    case DC_SIGCHAR_BOOL:
        return boolSV(dcCallBool(call->cvm, call->fptr));
    case DC_SIGCHAR_CHAR:
        return newSVnv((char)dcCallChar(call->cvm, call->fptr));
    case DC_SIGCHAR_UCHAR:
        return newSVuv((unsigned char)dcCallChar(call->cvm, call->fptr));
    case DC_SIGCHAR_SHORT:
        return newSViv((short)dcCallShort(call->cvm, call->fptr));
    case DC_SIGCHAR_USHORT:
        return newSVuv((unsigned short)dcCallShort(call->cvm, call->fptr));
    case DC_SIGCHAR_INT:
        return newSViv((int)dcCallInt(call->cvm, call->fptr));
    case DC_SIGCHAR_UINT:
        return newSVuv((unsigned int)dcCallInt(call->cvm, call->fptr));
    case DC_SIGCHAR_LONG:
        return newSViv((long)dcCallLong(call->cvm, call->fptr));
    case DC_SIGCHAR_ULONG:
        return newSVuv((unsigned long)dcCallLong(call->cvm, call->fptr));
    case DC_SIGCHAR_LONGLONG:
        return newSViv((long long)dcCallLongLong(call->cvm, call->fptr));
    case DC_SIGCHAR_ULONGLONG:
        return newSVuv((unsigned long long)dcCallLongLong(call->cvm, call->fptr));
    case DC_SIGCHAR_FLOAT:
        return newSVnv((float)dcCallFloat(call->cvm, call->fptr));
    case DC_SIGCHAR_DOUBLE:
        return newSVnv((double)dcCallDouble(call->cvm, call->fptr));
    case DC_SIGCHAR_POINTER:
        return sv_setref_pv(newSV(0), "Dyn::Call::Pointer", dcCallPointer(call->cvm, call->fptr));
    case DC_SIGCHAR_STRING:
        return newSVpv((const char *) dcCallPointer(call->cvm, call->fptr), 0);
    case DC_SIGCHAR_AGGREGATE: /* TODO: dyncall structs/union/array aren't ready upstream yet*/
        warn(
            "dyncall aggregate types (structs/union/array) aren't ready upstream yet at %s line %d",
            __FILE__, __LINE__);
        break;
    default:
        warn("Unknown return character: %c at %s line %d", call->ret, __FILE__, __LINE__);
    };
    return NULL;
}

// TODO: This might need to return values in arg pointers
#define _call_                                                                                     \
    if (call != NULL) {                                                                            \
        dcReset(call->cvm);                                                                         \
        push(aTHX_(Call *) call, ax);                                                               \
        /*warn("ret == %c", call->ret);*/                                                           \
        SV *ret = retval(aTHX_ call);                                                               \
        if (ret != NULL) {                                                                          \
            ret = sv_2mortal(ret);                                                                  \
            ST(0) = ret;                                                                           \
            XSRETURN(1);                                                                           \
        }                                                                                          \
        else                                                                                       \
            XSRETURN_EMPTY;                                                                         \
        /*//warn("here at %s line %d", __FILE__, __LINE__);*/                                      \
    }                                                                                              \
    else                                                                                           \
        croak("Function is not attached! This is a serious bug!");                                \
    /*//warn("here at %s line %d", __FILE__, __LINE__);*/

static Call *_load(pTHX_ DLLib *lib, const char *symbol, const char *sig) {
    if (lib == NULL) return NULL;
     warn("_load(..., %s, %s)", symbol, sig);
    Call *RETVAL;
    Newx(RETVAL, 1, Call);
    RETVAL->lib = lib;
    RETVAL->cvm = dcNewCallVM(1024);
    if (RETVAL->cvm == NULL) return NULL;
    RETVAL->fptr = dlFindSymbol(RETVAL->lib, symbol);

    if (RETVAL->fptr == NULL) // TODO: throw warning
        return NULL;
    size_t sig_len = strlen(sig);
    RETVAL->sig_len = sig_len;
    Newxz(RETVAL->sig, sig_len+1, char);

    CopyD(sig, RETVAL->sig, sig_len, char);
    Newxz(RETVAL->perl_sig, sig_len, char); // Dumb
    parse_signature(aTHX_ RETVAL);

    warn("Now: %s|%s|%c", RETVAL->perl_sig, RETVAL->sig, RETVAL->ret);
    return RETVAL;
}




MODULE = Dyn PACKAGE = Dyn

void
test(AV * values, int lettter, CV * code)
CODE:
    ;

void
DESTROY(...)
CODE:
    Call * call;
    call = (Call *) XSANY.any_ptr;
    if (call == NULL)      return;
    if (call->lib != NULL) dlFreeLibrary( call->lib );
    if (call->cvm != NULL) dcFree(call->cvm);
    if (call->sig != NULL)       Safefree( call->sig );
    if (call->perl_sig != NULL ) Safefree( call->perl_sig );
    Safefree(call);

void
call_Dyn(...)
PPCODE:
    Call * call = (Call *) XSANY.any_ptr;
     warn("here at %s line %d", __FILE__, __LINE__);
    _call_
     warn("here at %s line %d", __FILE__, __LINE__);

SV *
wrap(lib, const char * func_name, const char * sig, ...)
CODE:
    DLLib * lib;
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Dyn::DLLib")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else
        lib =
#if defined(_WIN32) || defined(_WIN64)
        dlLoadLibrary( (const char *) SvPV_nolen(ST(0)) );
#else
        (DLLib*)dlopen( (const char *) SvPV_nolen(ST(0)), RTLD_LAZY/* RTLD_NOW|RTLD_GLOBAL */);
#endif
    Call * call = _load(aTHX_ lib, func_name, sig);
    CV * cv;
    STMT_START {
        cv = newXSproto_portable(NULL, XS_Dyn_call_Dyn, (char*)__FILE__, call->perl_sig);
        if (cv == NULL)
            croak("ARG! Something went really wrong while installing a new XSUB!");
        XSANY.any_ptr = (void *) call;
    } STMT_END;
    RETVAL = sv_bless(newRV_inc((SV*) cv), gv_stashpv((char *) "Dyn", 1));
OUTPUT:
    RETVAL

void
call_attach(Call * call, ...)
PPCODE:
    ////warn("call_attach( ... )");
    _call_

SV *
attach(lib, const char * symbol_name, const char * sig, const char * func_name = NULL)
PREINIT:
    Call * call;
    DLLib * lib;
    //  $lib_file, 'add_i', '(ii)i' , '__add_i'
CODE:
    if (SvROK(ST(0)) && sv_derived_from(ST(0), "Dyn::DLLib")) {
        IV tmp = SvIV((SV*)SvRV(ST(0)));
        lib = INT2PTR(DLLib *, tmp);
    }
    else
        lib =
#if defined(_WIN32) || defined(_WIN64)
        dlLoadLibrary( (const char *) SvPV_nolen(ST(0)) );
#else
        (DLLib*)dlopen( (const char *) SvPV_nolen(ST(0)), RTLD_LAZY/* RTLD_NOW|RTLD_GLOBAL */);
#endif
    ////warn("ix == %d | items == %d", ix, items);
    if (func_name == NULL)
        func_name = symbol_name;

    call = _load(aTHX_ lib, symbol_name, sig);
    if (call == NULL)
        croak("Failed to attach %s", symbol_name);
    /* Create a new XSUB instance at runtime and set it's XSANY.any_ptr to contain the
     * necessary user data. name can be NULL => fully anonymous sub!
    **/
    CV * cv;
    STMT_START {
        cv = newXSproto_portable(func_name, XS_Dyn_call_Dyn, (char*)__FILE__, call->perl_sig);
        ////warn("N");
        if (cv == NULL)
            croak("ARG! Something went really wrong while installing a new XSUB!");
        ////warn("Q");
        XSANY.any_ptr = (void *) call;
    } STMT_END;
    RETVAL = newRV_inc((SV*) cv);
OUTPUT:
    RETVAL

void
__install_sub( char * package, char * library, char * library_version, char * signature, char * symbol, char * full_name )
PREINIT:
    Delayed * _now;
    Newx(_now, 1, Delayed);
CODE:
    Newx(_now->package, strlen(package) +1, char);
    memcpy((void *) _now->package, package, strlen(package)+1);
    Newx(_now->library, strlen(library) +1, char);
    memcpy((void *) _now->library, library, strlen(library)+1);
    Newx(_now->library_version, strlen(library_version) +1, char);
    memcpy((void *) _now->library_version, library_version, strlen(library_version)+1);
    Newx(_now->signature, strlen(signature)+1, char);
    memcpy((void *) _now->signature, signature, strlen(signature)+1);
    Newx(_now->symbol, strlen(symbol) +1, char);
    memcpy((void *) _now->symbol, symbol, strlen(symbol)+1);
    Newx(_now->name, strlen(full_name)+1, char);
    memcpy((void *) _now->name, full_name, strlen(full_name)+1);
    _now->next = delayed;
    delayed = _now;

void
END( ... )
PPCODE:
    Delayed * holding;
    while (delayed != NULL) {
        //warn ("killing %s...", delayed->name);
        Safefree(delayed->package);
        Safefree(delayed->library);
        Safefree(delayed->library_version);
        Safefree(delayed->signature);
        Safefree(delayed->symbol);
        Safefree(delayed->name);
        holding = delayed->next;
        Safefree(delayed);
        delayed = holding;
    }

void
AUTOLOAD( ... )
PPCODE:
    char* autoload = SvPV_nolen( sv_mortalcopy( get_sv( "Dyn::AUTOLOAD", TRUE ) ) );
    ////warn("$AUTOLOAD? %s", autoload);
    {   Delayed * _prev = delayed;
        Delayed * _now  = delayed;
        while (_now != NULL) {
            if (strcmp(_now->name, autoload) == 0) {
                warn(" signature: %s", _now->signature);
                warn(" name:      %s", _now->name);
                warn(" symbol:    %s", _now->symbol);
                warn(" library:   %s", _now->library);
                if (_now->library_version != NULL)
                    warn (" version:  %s", _now->library_version);
                warn(" package:   %s", _now->package);
                SV * lib;
                //if (strstr(_now->library, "{")) {
                    char eval[1024]; // idk
                    sprintf(eval, "package %s{sub{sub{Dyn::guess_library_name(%s,%s)}}->()->();};",
                        _now->package, _now->library,
                        _now->library_version
                    );
                    warn("eval: %s", eval);
                    lib = eval_pv( eval, FALSE ); // TODO: Cache this?
                    warn("after eval, lib == %s", SvPV_nolen(lib));
                //}
                //else
                //    lib = newSVpv(_now->library, strlen(_now->library));
                //SV * lib = get_sv(_now->library, TRUE);
                //warn("     => %s", (const char *) SvPV_nolen(lib));
                char *sig, ret, met;
                const char * lib_name = SvPV_nolen(lib);
                DLLib * _lib =
#if defined(_WIN32) || defined(_WIN64)
                    dlLoadLibrary( lib_name );
#else
                    (DLLib*)dlopen( lib_name, RTLD_LAZY/* RTLD_NOW|RTLD_GLOBAL */);
#endif
                if (_lib == NULL) {
#if defined(_WIN32) || defined(__WIN32__)
                unsigned int err = GetLastError();
                warn ("GetLastError() == %d", err);
#endif
                    croak("Failed to load %s", lib_name);
                }
                Call * call = _load(aTHX_ _lib, _now->symbol, _now->signature );

                //warn("Z");
                if (call != NULL) {
                    CV * cv;
                    //warn("Y");
                    STMT_START {
                       // //warn("M");
                        cv = newXSproto_portable(autoload, XS_Dyn_call_Dyn, (char*)__FILE__, call->perl_sig);
                        ////warn("N");
                        if (cv == NULL)
                            croak("ARG! Something went really wrong while installing a new XSUB!");
                        ////warn("Q");
                        XSANY.any_ptr = (void *) call;
                        ////warn("O");
                    } STMT_END;
                    ////warn("P");
                    ////warn("AUTOLOAD( ... )");
                    _call_
                    _prev->next = _now->next;

                    Safefree(_now->library);
                    Safefree(_now->package);
                    Safefree(_now->signature);
                    Safefree(_now->symbol);
                    Safefree(_now->name);
                    Safefree(_now);
                }
                else
                    croak("Oops!");
                ////warn("A");
                //if (_prev = NULL)
                //    _prev = _now;
                return;
             }
            _prev = _now;
            _now  = _now->next;
        }
    }
    die("Undefined subroutine &%s", autoload);

BOOT:
    delayed = NULL;




MODULE = Dyn    PACKAGE = Dyn

void
sublike(kwname, body)
    char *kwname
    CV   *body
  INIT:
    SV *key;
    HE *he;
  CODE:
    warn("=== %s", __FUNCTION__);

    key = newSVpvf("%s", kwname);
    sv_2mortal(key);

    he = hv_fetch_ent(active_keywords_hv(), key, 1, 0);
    HeVAL(he) = SvREFCNT_inc(body);

    /* TODO: SAVE a destructor for end of scope to remove it again
     */



HV *
___defined_aggrs()
PREINIT:
    dMY_CXT;
CODE:
    RETVAL = newHVhv(MY_CXT.classes);
OUTPUT:
    RETVAL


BOOT:
  //wrap_keyword_plugin(&my_keyword_plugin, &next_keyword_plugin);


    MY_CXT_INIT;
    MY_CXT.count = 0;
    MY_CXT.classes = newHV();



MODULE = Dyn    PACKAGE = Dyn::Call::Aggregate

bool
_define_aggregate(const char * name, AV * types, bool packed = false)
CODE:
    warn("Hello, world!");
    RETVAL = true;
OUTPUT:
    RETVAL
