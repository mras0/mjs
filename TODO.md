* Lexer
    - Use `wstring_view` for token text (it's kept alive by the `source_file`)
    - Better handling of e.g. number literals..
    - Missing stuff?
* Parser
    - `for in` / `with` statements
    - Create `parse_test` (and move `test_semicolon_insertion` from interpreter test)
    - Test `source_extend` logic (could probably be a lot more precise for expressions/statements)
* Better GC
    - Add more leak checks
    - And test current GC with live objects
    - Make thread-safe (probably need to pass around a context variable)
* Finish global object
    - Date
        - mutators
        - time zone adjustments
        - to/from string
    - Function
        - toString
        - etc.
* Optimize NumberToString()
* Create example(s)
    - Embedding mjs (I.e. adding user-defined classes)
* Make it easy to evaluate javascript expressions (e.g. String('12') + 34)
* Optimize nested functions (they only need to be processed once (?))
* AST optimizations (constant propagation etc.)
* Optimize Array(), etc.
* Compile under Linux
* Use `char16_t` instead of `wchar_t` (wstring/wistringstream etc. etc.)
* Avoid duplicating `duration_cast`/`typeid()` logic with `expression_type`/`statement_type`. Consider using std::variant.
* General refactoring, implement ES3, ES5, ES...., JIT, etc. :)
