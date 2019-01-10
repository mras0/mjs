* Lexer
    - Follow specification exactly when handling e.g. number literals..
    - Add more test cases...
    - Proper UTF-16 support
* Parser
    - Test `source_extend` logic (could probably be more precise for expressions/statements)
    - More tests...
* Interpreter
    - Lessen stack usage
    - Make sure deep recursion is supported without running out of space
    - Handle error conditions better
    - Make sure nested function definitions aren't processed multiple times
    - Optimize `NumberToString()` (and related functions)
    - Optimize nested functions (they only need to be processed once (?))
    - AST optimizations (constant propagation etc.)
    - Escape analysis
    - Make it easier/possible to directly throw the correct error types (i.e. remove need to `to_primitive_failed_error` and `not_callable_exception`)
 * Global object
    - Date
        - time zone adjustments
        - locale functions
    - RegExp
        - multiline regexp
        - ES3 regular expression literals shouldn't be unique
    - Create function objects on demand (?)
    - Consider implementing some things in ECMAScript (i.e. polyfill it)!
* Better GC
    - Ensure exception safety
    - Support growing the heap (and support growing it as needed)
    - Could probably support generational GC by parititioning one big `storage_` into multiple little "sub heaps"
    - Ensure thread safety (probably don't allow sharing heaps between threads at first)
    - Improve speed
    - Support compacting the current heap? Should be possibly by making changes in `gc_heap` exclusively (other parts of the system shouldn't need to be changed)
    - Make it harder to use incorrectly - more type safety possible?
    - Support pointers inside objects (like `shared_ptr`s aliasing constructor)
    - Allocator support (But it seems like `gc_heap_ptr` is too fancy to be compatible - a static `to_pointer()` function can't really be 'nicely' [it could of course use `local_heap`, but that's not nice])
    - Support use of multiple heaps (for generational GC)
    - It's probably possible to optimize cleanup of tracked pointer - at the end of `garbage_collect` we should know which pointers are getting detached, temporarily turn `deatch` into a NO-OP and just clear the part of the `pointers_` array we know is going to be destructed.
    - Experiment (again) with reference counting the object and string references stored in `value`
    - Add tests ! (for `value_representation`, all the pointer types etc.)
    - Perhaps using structure of arrays instead of array of structures for `gc_type_info` could give (marginal) speed benefits
    - Move `string_cache` to its own file and create test for `string_cache` (and weak pointers in general)
    - Consider making `gc_table` available for geenral use (e.g. by `string_cache`)
    - Have `native_object::add_native_property` take the name as a non-type template parameter and do some constexpr stuff (or something)
    - `native_object` should handle the issues faced by`activation_object` and `array_object` or probably die (or be repurposed)
    - Reduce number of registered classes by moving `gc_table` out of `gc_vector` (and possibly making it type erased?)
    - Handle interface types better (currently only `object` and derived classes are handled)
    - Help debug missing calls to `{native_}object::fixup()` in derived classes
    - Passing const references to `gc_heap_ptr`s can be dangerous when calling out to functions that might trigger GC!
* REPL
    - Add tests
    - Add support for specifying the wanted ECMAScript version as a commandline argument
* Create example(s)
    - Embedding mjs (I.e. adding user-defined classes)
* Make `string` easier to use - without going back to having a static `local_heap`
    - make `string_builder` class or drop support for operator+?
* Make it easy to evaluate (and inspect) Javascript expressions (e.g. String('12') + 34)
* Use `char16_t` instead of `wchar_t` (and update `wstring`/`wistringstream` etc. etc.)
* Avoid duplicating `duration_cast`/`typeid()` logic with `expression_type`/`statement_type`. Consider using std::variant.
* General refactoring, implement ES5, ES...., JIT, etc. :)
