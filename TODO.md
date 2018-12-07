* Lexer
    - Use `wstring_view` for token text (it's kept alive by the `source_file`)
    - Better handling of e.g. number literals..
    - Missing stuff?
* Parser
    - Create `parse_test` (and move `test_semicolon_insertion` from interpreter test)
    - Test `source_extend` logic (could probably be more precise for expressions/statements)
* Better GC
    - Ensure exception safety
    - Support growing the heap (and support growing it as needed)
    - Do real semi-space collector - I.e. double the size of `storage_` but only fill it half way through, switching between halfs when one gets full
        - Could probably support this and generational GC by parititioning one big `storage_` into multiple little "sub heaps"
    - Ensure thread safety (probably don't allow sharing heaps between threads at first)
    - Improve speed
    - Support compacting the current heap? Should be possibly by making changes in `gc_heap` exclusively (other parts of the system shouldn't need to be changed)
    - Make it harder to use incorrectly - more type safety possible? More help for implementing `gc_table`/`gc_function`-like objects
    - Support pointers inside objects (like `shared_ptr`s aliasing constructor)
    - Allocator support (But it seems like `gc_heap_ptr` is too fancy to be compatible - a static `to_pointer()` function can't really be 'nicely' [it could of course use `local_heap`, but that's not nice])
    - Support use of multiple heaps (for generational GC)
    - It's probably possible to optimize cleanup of tracked pointer - at the end of `garbage_collect` we should know which pointers are getting detached, temporarily turn `deatch` into a NO-OP and just clear the part of the `pointers_` array we know is going to be destructed.
    - Experiment (again) with reference counting the object and string references stored in `value`
    - Add tests ! (for `value_representation`, all the pointer types etc.)
    - Perhaps using structure of arrays instead of array of structures for `gc_type_info` could give (marginal) speed benefits
* Finish global object
    - Date
        - mutators
        - time zone adjustments
        - to/from string
    - Probably other missing stuff and non-compliant implementations...
    - Split into multiple classes/files
    - Optimize `array_object` - may need to make more `object` functions virtual and do the same for string objects
    - Add test cases for directly relatable to clauses in the spec (e.g. `Boolean.prototype.constructor` exists and is correct)
* Interpreter
    - Lessen stack usage
    - Make sure deep recursion is supported without running out of space
    - Handle error conditions better
    - Make sure nested function definitions aren't processed multiple times
* REPL
    - Add tests
    - Garbage collect "sometimes"
* Optimize `NumberToString()`
* Create example(s)
    - Embedding mjs (I.e. adding user-defined classes)
* Make `string` easier to use - without going back to having a static `local_heap`
    - make `string_builder` class or drop support for operator+?
* Make it easy to evaluate Javascript expressions (e.g. String('12') + 34)
* Optimize nested functions (they only need to be processed once (?))
* AST optimizations (constant propagation etc.)
* Optimize Array(), etc.
* Compile under Linux
* Use `char16_t` instead of `wchar_t` (and update `wstring`/`wistringstream` etc. etc.)
* Avoid duplicating `duration_cast`/`typeid()` logic with `expression_type`/`statement_type`. Consider using std::variant.
* General refactoring, implement ES3, ES5, ES...., JIT, etc. :)
