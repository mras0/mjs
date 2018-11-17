* Lexer
    - Use `wstring_view` for token text (it's kept alive by the `source_file`)
    - Better handling of e.g. number literals..
    - Missing stuff?
* Parser
    - Create `parse_test` (and move `test_semicolon_insertion` from interpreter test)
    - Test `source_extend` logic (could probably be more precise for expressions/statements)
* Better GC
    - Make sure garbage can be collected safely most of the time (and formulate rules for when it's not allowed)
    - Support growing the heap (and support growing it as needed)
    - Ensure thread safety (perhaps only GC type information registration is troublesome)
    - Improve speed
    - Support compacting the current heap? Should be possibly by making changes in `gc_heap` exclusively (other parts of the system shouldn't need to be changed)
    - Make it harder to use incorrectly - more type safety possible? More help for implementing `gc_table`/`gc_function`-like objects
    - Support pointers inside objects (like `shared_ptr`s aliasing constructor)
    - Allocator support (But it seems like `gc_heap_ptr` is too fancy to be compatible - a static `to_pointer()` function can't really be 'nicely' [it could of course use `local_heap`, but that's not nice])
    - Support use of multiple heaps (for generational GC)
    - Support weak pointers into the GC heap (flag on `gc_heap_ptr_untracked`?) - could be useful for string caches (in `global_object`?)
    - It's probably possible to optimize cleanup of tracked pointer - at the end of `garbage_collect` we should know which pointers are getting detached, temporarily turn `deatch` into a NO-OP and just clear the part of the `pointers_` array we know is going to be destructed.
* Finish global object
    - Date
        - mutators
        - time zone adjustments
        - to/from string
    - Probably other missing stuff and non-compliant implementations...
    - Implement (more) things as "real" C++ classes (?)
    - Split into multiple classes/files
* Interpreter
    - Lessen stack usage
    - Make sure deep recursion is supported without running out of space
    - Handle error conditions better
    - Make sure nested function definitions aren't processed multiple times
* Optimize `NumberToString()`
* Create example(s)
    - Embedding mjs (I.e. adding user-defined classes)
* Make `string` easier to use - without going back to having a static `local_heap`
* Make it easy to evaluate Javascript expressions (e.g. String('12') + 34)
* Optimize nested functions (they only need to be processed once (?))
* AST optimizations (constant propagation etc.)
* Optimize Array(), etc.
* Compile under Linux
* Use `char16_t` instead of `wchar_t` (and update `wstring`/`wistringstream` etc. etc.)
* Avoid duplicating `duration_cast`/`typeid()` logic with `expression_type`/`statement_type`. Consider using std::variant.
* General refactoring, implement ES3, ES5, ES...., JIT, etc. :)
