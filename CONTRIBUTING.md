# How to contribute

I'm really glad you're reading this, because I need volunteers who are smarter than I am to help this project come to fruition.

I want to make contributing to this project as easy as possible so please take note of the following resources:

  * [Issue tracker](https://github.com/sanko/Affix.pm/issues)
  * [My roadmap](https://github.com/sanko/Affix.pm/discussions/41)
  * [Pull request queue](https://github.com/sanko/Affix.pm/pull/new/master)

My email address is public and the account is checked regularly but I prefer to communicate via the above links for the sake of transparency. 

# Submitting changes

I intend to use the '[Github flow](https://docs.github.com/en/get-started/using-github/github-flow)' to maintain this project and invite you to submit changes the same way:

  1. [Fork the repository](https://github.com/sanko/Affix.pm/fork) under your account or organization
  2. Clone the repo to your local machine with `git clone https://github.com/your-account-here/Affix.pm.git`
  3. Create a new working branch based on `main` with `git checkout -b brange-name-here`
  4. Get to work!  
  5. If you've added code, please verify functionality with tests. Leak tests are especially welcome.
  6. If you changed any of the API, please update the documentation, provide tests, and explain the changes in your commit and/or pull request message.
  7. If possible, please follow my coding conventions (see below) and make sure all of your commits are atomic (one feature per commit).
  8. Add changed files with `git add insert-paths-of-changed-files-here`, commit the changes with `git commit -m "Insert a short message of the changes made here"`, and push the changes to the remote repo using `git push origin branch-name-here`
  9. Verify the entire test suite passes on the 'big 3' (Windows, macOS, Linux) by allowing the Github Actions matrix to run. 
  10. If everything checks out, issue the pull request in the Github interface

## Coding conventions

I optimize for (my own) readability:

  * Use 4-space indent with tabs disabled
  * Keep lines under 120 characters
  * Aim to use self-documenting variable names
  * C++17 is the current target but C++20 (or higher) will become the base if I'm convinced a new feature would make my life easier
  * Aim to support the last two or three major versions of perl. Breaking changes that lose releases older then six years for the sake of language features are fine and encouraged
  * Wrap braces around control statements, but not empty blocks
  * Put spaces after list items and method parameters (`1, 2, 3`, not `1,2,3`), around fat commas (`key => 'value'`, not `key=>'value'`), and around operators (`x += 1`, not `x+=1`)
  * Separate logical blocks with a blank line
  * Break before multi-line strings and long functions/loops/ifs
  * Use consistent comment formatting and spacing
  * Aim for the big 3 (Linux, macOS, Windows) platform support without large `#if ... #endif` conditional blocks
  * If submitted documentation or code examples only work on a subset of the 'big 3', please mention that; BSDs, etc. are nice but not an immediate requirement for submitting a pull request

Most of the code layout stuff can be handled with the included `.clang-format` and `.tidyallrc` files.

## License and Legal

When contributing to this project, you must agree that you have authored 100% of the content, that you have the necessary rights to the content and that the content you contribute may be provided under the [The Artistic License 2.0](https://github.com/sanko/Affix.pm/blob/main/LICENSE).

# Reporting Issues

When reporting issues, please use the [public tracker](https://github.com/sanko/Affix.pm/issues) here on Github rather than submitting them to my email address.

Please do your absolute best to explain the problem clearly. Some helpful information includes:

 * A description of your platform including OS version and processor type (Windows, Linux, macOS, x86, ARM, etc.)
 * Your perl version
 * As much information you have about the library you're attempting to wrap
 * Any example code you have. A minimal example would be fantastic
 * Tell me what you expected to happen
 * Show what happens
 * Include what else you've tried, if the code works on another platform, etc.
 * Any added notes that might be helpful
 
 Be thorough!
