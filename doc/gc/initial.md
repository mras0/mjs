# Implementing a Garbage Collector in C++

## TL;DR

- Main garbage collector source code: [`gc_heap.h`](../../src/mjs/gc_heap.h) [`gc_heap.cpp`](../../src/mjs/gc_heap.cpp) 
- Scroll to the bottom for a link to a visualization showing the garbage
  collector in action (it may also provide some clarity if the text is
  vague)

## Introduction

In this document I'll attempt to give an overview of the new
garbage collector (GC) I've implemented for my hobby [ECMAScript
1997](https://www.ecma-international.org/publications/files/ECMA-ST-ARCH/ECMA-262,%201st%20edition,%20June%201997.pdf)
interpreter [mjs](https://github.com/mras0/mjs). Even though it was
created for that specific purpose the principles  - and code, with some
adjustments - should be generally applicable.

To keep it relatively short I'll assume a basic familiarity with the
concept of garage collection (otherwise
[Wikipedia](https://en.wikipedia.org/wiki/Garbage_collection_(computer_science))
is a decent starting point) as well as a working knowledge of C++. I'll
try to limit the amount of C++ cruft and esoterica discussed here and
instead refer the interested reader to the source code.

My implementation is inspired by an excellent talk given by Herb Sutter
at CppCon 2016 titled "Leak-Freedom in C++... By Default"
([video](https://www.youtube.com/watch?v=JfmTagWcqoE))
([slides](https://github.com/CppCon/CppCon2016/blob/master/Presentations/Lifetime%20Safety%20By%20Default%20-%20Making%20Code%20Leak-Free%20by%20Construction/Lifetime%20Safety%20By%20Default%20-%20Making%20Code%20Leak-Free%20by%20Construction%20-%20Herb%20Sutter%20-%20CppCon%202016.pdf))
and NaN tagging as described by Mike Pall in the following post to the
[LuaJIT](http://luajit.org/) mailing list: ["LuaJIT 2.0 intellectual property disclosure and research opportunities"](http://lua-users.org/lists/lua-l/2009-11/msg00089.html).

While I consider the implementation good enough (in this project, for
now) it's far from being "production ready" and you definitely shouldn't
be using it for anything you or anyone else rely on :). See my [TODO
list](https://github.com/mras0/mjs/blob/master/TODO.md) for a small
sampling of what I know needs improvement.

## Background

Before diving into the details, let's have a look at the problem we're
trying to solve.

Why does an ECMAScript interpreter need a GC in the first place? Who
isn't cleaning up after themselves and causing us to have to expend
extra clock cycles dealing with something that's usually handled fairly
automatically in modern C++?

### Values

To answer that we first need to talk about *values*.

In ECMAScript 1997 a value can have one of the following types:

  - undefined
  - null
  - boolean
  - number
  - string
  - object

Note: In ECMAScript a value always has a single unchanging type, it's
the fact that variables and object properties can be assigned values
of differing types that makes it a dynamically typed language.

The first four on the list are easily handled, and since strings are
immutable and can't contain references to anything else these five types are
collectively called *primitive* by the ECMAScript specification. They are not
the ones causing issues.

### Objects

That leaves the culprit: object. "What's in an object?", you might ask.
Well, it's really just a collection of properties. Some of them are - as
an implementation detail - treated specially, like for instance the
prototype(\*) or the name of the objects class, but essentially objects are
just a lists of properties.  A property of an object consists of its
name/key (a string), its attributes (irrelevant for this discussion) and
its value.

Now you may be seeing where this is going: Objects contain properties
that contain values that may contain objects (or rather references to
objects). This is at the root of the issue.

\*: The internal prototype called `[[Prototype]]` in the specification. Not
to be confused with the `prototype` property you'll encounter when
dealing with function objects.

## Inadequacies of the Initial Garbage Collector

My general implementation strategy has been that of walking the path of
least resistance unless I found enjoyment in diving into a specific
area, so my initial version of the interpreter used the "obvious" C++
primtives.

This means that the interesting types were represented as follows:

- objects references as `std::shared_ptr<mjs::object>`, where
  `mjs::object` manages a standard container of properties (key/value
  pairs)
- values as the aptly named `mjs::value` - a classic [tagged
  union](https://en.wikipedia.org/wiki/Tagged_union) capable of storing
  object references as well as the other ECMAScript primitives (numbers,
  strings, etc.)

The limitations of my initial approach to GC were seen when it was
confronted with this simple piece of ECMAScript code:
```javascript
var o = new Object(); // Create object 'o'. Note: no object initialiser/literal until ES3
o.p = o;              // Set property 'p' of object 'o' to point to 'o' itself
```

And lest you think this only occurs in pathological cases consider the
constructor property of a functions prototype. It will contain a
reference to the prototype object, which contains a reference to the
constructor which...

As `std::shared_ptr` provides garbage collection through reference
counting the initial implementation was faced with handling the
classic issue of dealing with self references. The problem being that
in the above example, even if there are no external references, the
reference count of `o` will always be positive because of the
reference from its own property `p`. This example is simplified but
the problem persists even if the chain is longer.

The initial implementation handled this rather crudely by - when
requested - going through all objects and clearing their internal
property lists thereby ensuring that no self references existed. By
manually breaking any cycles that might otherwise keep an object from
being destructed leaks were avoided.

As you can imagine this was only requested when testing whether there
were other leaks in the system as it was a hugely wasteful pessimization
compared to just using the [null
collector](https://blogs.msdn.microsoft.com/oldnewthing/20100809-00/?p=13203).
The initial GC was especially useless since I hadn't bothered
implementing detection of live objects, so it couldn't even be used
while the interpreter was running.

## The Set of Live Objects

At the heart of garbage collection lies the task of determining which
objects are *live*, i.e. which objects cannot be collected.
`std::shared_ptr` does this by keeping track of the number of references
to the object, but as mentioned above this alone isn't sufficient due to
cycles in the object graph. Edges in the object graph are formed by
references in the property list to other objects and a cycle implies
that you can reach an object by following its outgoing edges back to
itself - like in the simplified example mentioned earlier.

In this context "live" means that we will possible be needing the object
again in the future, but how can that be determined without predicting
the future? In a bit of a cop-out garbage collection has the concept of
*root pointers* which are a special class of object reference from
the "outside" which force an object to kept alive even if it wouldn't
otherwise have been. In simple terms a root pointer can be thought of as
something the "user" can refer to - like `o` in the example or the
`window` object when used in a web browser context.

Given the root pointers determining the set of live objects is
conceptually easy enough: it's the set of all objects that are
*reachable* starting from the root pointers. Or in pseudo-code:

```
    find-live(root-pointers)
        for o in root-pointers
            mark-live(o)

    mark-live(o)
        if contains(live-set, o) # Avoid infinite recursion
            return
        insert(live-set, o)
        for p in properties-of(o)
            if p is object-reference
                mark-live(p)
```

So by keeping track of the root pointers and going from there we should
be able to create a  garbage collection for our interpreter.

## The New Garbage Collector

To actually implement a garbage collector we'll have to become more
specific about what a root pointer is and start discussing the
implementation. I'll be presenting the details without mentioning all
the different trade-offs that were considered, but be aware that for
nearly every decision there are lots of other interesting possibilities
to explore.

### Requirements

Before starting the implementation I had a set of requirements in mind:

- The collector should be [precise](https://en.wikipedia.org/wiki/Tracing_garbage_collection#Precise_vs._conservative_and_internal_pointers)
- It shouldn't introduce too many changes in the existing
  interpreter
- It shouldn't rely on non-portable constructs
- It should be type-safe to greatest possible degree
- It should be exception safe
- It should be possible to make thread-safe
- It should be optimizable (meaning: it can be inefficient to start off
  with but it should be possible to make it fast at a later pointer)
- It should be extensible - I'd like to be able to use the code as a
  testbed for exploring GC ideas
- It should be possible to move objects in memory (to be able to compact
  the heap for example)
- It doesn't have to be generally useable, but it should be separable
  from the interpreter
- It should be possible to keep the amount of memory used by the objects
  to a fixed number of bytes

### The GC Heap

It was pretty clear from the start that I needed a special heap where
objects could be stored. It both helps with the requirement to be able
to restrict the amount of memory used for objects and in general
simplifies things when it's known that all valid objects live within a
certain memory range.

In the code this is represented by the `mjs::gc_heap` class which
handles storage management and object lifetime. Along with a set of
helper (template) classes it handles everything related to the garbage
collector. The raw storage is allocated in 64-bit chunks (to match the
alignment requirement of pointers on 64-bit systems and numbers) called
*slots*.

The public interface of `gc_heap` only permits construction of objects
inside the managed heap and requesting a garbage collection. Everything
else is handled automatically.

Each allocation in the heap is preceded by a `slot_allocation_header`
which is an extra slot of overhead that records the number of slots
allocated (including the header) as well as the *type index* of the
object that lives there.

Storing the size of each allocation enables traversing the heap and
having a type associated turns out to be pretty useful (read:
essential).

### Other Types on the GC Heap

Even though we don't strictly need to store anything other than objects
on the garbage collected heap, it would be really convenient to be able
to have all (or most) things related to the interpreter state kept there
as well. Like all the strings created during execution for example.

More importantly *activation records* (loosely "the call stack") and
function implementations can both contain self-references and it
simplifies things a lot if they can be treated as normal ECMAScript
objects by the GC even if they're not implement the same way.

Hence the need for distinguishing between different kinds of "things"
allocated on the heap via the type index.

Note: From now on I'll try to use `object` when I mean an ECMAScript
object and just object when I mean a C++ object that can be allocated on
the heap.

### GC Type Information

Except for special cases that will be discussed later the type index
associated with an allocation can be used to obtain the statically
initialized `mjs::gc_type_info` instance describing the type.

The type information object allows the garbage collector to destroy,
move and perform the other necessary operations on anything that lives
at a given storage position.

The same thing could be accomplished by requiring all objects
constructed on the GC heap to derive from some abstract base class, but
where is the fun in that? (Also some optimizations wouldn't be possible.)

### Tracked Pointers

To keep track of which objects are live, the GC needs to know whether
there are any active references to them. To be able to move objects around
in memory during garbage collection there also needs to be a layer of
indirection for the clients, so they can remain (mostly) unaware that the
pointer-to object has been relocated.

For these reasons `gc_heap_ptr` exists and serves the same purpose as
`std::shared_ptr` in the old implementation. It's also a smart pointer
class template, but instead of increment/decrementing a reference count
on construction/destruction it registers/deregisters itself with its
associated `gc_heap`.

This way the GC heap can manage the storage as it sees fit and
have everything "just work" by making sure all the tracked pointers are
updated when it moves things around. If, at any point, it can determine
that there are no pointers to an object it can safely delete it. Of
course that will only make the garbage collector as good as the old
implementation.

### Determining the Root Pointers

To do better we need to determine the root pointers and use the
algorithm sketched above and do proper garbage collection of anything
that isn't reachable from them.

By requiring that all objects on the GC heap store their pointers on the
heap as well it's easy to determine the root pointers: they're all
pointers which addresses lie outside the managed heap. This requirement
also enables another important feature: for any object we can determine
what other object it references by considering all the tracked pointers
that are located inside its designated storage.

### Compacting

With everything in place we're ready to handle requests to perform a
garbage collection! This is done by moving all live objects to a new
storage location and destroying all the remaining objects. The new
storage will be compact and easy to allocate from, and not copying the
left-over objects is the garbage collection. Calling the destructor on
the garbage objects is what's referred to as *finilization* in many
languages.

The method described above:

1. From the root pointers determine the set of live objects
2. Move the live objects to somewhere else
3. Destroy any left-over objects
4. Use "somewhere else" as new storage area

while using tracked pointers and all the other machinery does work, but
it's not very efficient.

## Optimizations

To make sure the GC was relatively robust I ran the garbage
collector after every statement when running some parts of the test
suite. While this uncovered some bugs (see the Pitfalls section) it also
made the tests take a lot longer to run.

Profiling showed that (copy) constructing and destroying `gc_heap_ptr`
instances as well as processing tracked pointers in general was taking
up most of the time. This matched my intuition, but I made sure to
collect data before jumping to conclusions.

### A Backdoor

In the current implementation tracked pointers contain a pointer to the
containing `gc_heap` and a slot index into the storage area enabling it
to always produce a pointer to the pointed-to object at its current
storage location.

In numbers they take up lots of space and are very costly to move(\*),
so to enable optimizations at the expense of type-safety (please let me
know if there is a better way) objects are allowed to use dangerous -
but fast - methods of keeping track of object references.

\*: Since the address at which a tracked pointer lives is important it
isn't cheaper to move than to copy.

### Untracked Pointers

To limit these type-indiscretions the unsafe operations are accessed
by using `gc_heap_ptr_untracked` which only stores the slot index and
relies on the class implementer (since the user shouldn't be exposed to
it) supplying the `gc_heap`.

Since this presents a problem when the object is moved - the GC by
definition isn't tracking the pointer - a protocol is in place where the
garbage collector notifies the moved object that it wants to be informed
about its untracked pointers.

### Fixup

The *fixup* protocol is a way of ensuring the work that would otherwise
be automatically done when using tracked pointers can be manually
performed by clever classes that use untracked pointers. After an object
has been moved the garbage collector calls the objects `fixup` method
(if it has one with a matching signature). The object can then go
through its untracked pointers and fix up each in turn.  Calling `fixup`
on an untracked pointer ensures that whatever the pointer refers to is
also moved and that the pointer value is updated.

It's important that classes using untracked pointers provide a `fixup`
method that fixes each untracked pointer up exactly once. Forgetting to
fix up a pointer results in a dangling pointer and calling fixup more
than once (or outside the fixup method) will at best result in a crash.
This is the dangerous part of untracked pointers.

### Value Representation

Another unsafe helper class is `value_representation` which takes the
same amount of storage as a number (a [IEEE 754 double precision
floating point
number](https://en.wikipedia.org/wiki/Double-precision_floating-point_format))
and can hold any ECMAScript value. Numbers themselves are stored using
their normal representation (with all NaN values forced to a single
representation) and the remaining types are encoded using [NaN
tagging](http://lua-users.org/lists/lua-l/2009-11/msg00089.html).

For values of type string or object the pointer value is stored
untracked in the same way as `gc_heap_ptr_untracked` and also requires
a `fixup` after move.

### Strings and Tables

Strings are stored as a single allocation with a small header containing
the length followed by the UTF-16 characters it contains. Nothing fancy
to see there, except a possible optimization where it could deduce its
length from the allocation header. Some trickery would be needed to
get the length modulo 4, but that could be handled if necessary.

Since the properties of an `object` can change, they are kept in a
separate table that can be stored as a single allocation. When the table
needs to grow a copy of greater size is created and the reference to the
old table is simply forgotten (yay garbage collection). The table
entries are internally represented by an untracked pointer to the key (a
string), the attributes associated with the entry (a bit field), and a
`value_representation` to store the actual value. All together this
allows a compact representation where each entry only takes up two heap
slots (i.e. 128 bits).

### Representing the Set of Live Objects

During garbage collection the set of live objects isn't stored
explicitly. Instead the garbage collector keeps a stack of pointers that
are waiting to be processed (the normal stack isn't used to avoid
overflowing it when processing long object chains).

The stack is initially populated by the root pointers and when
processing a pointer the garbage collector performs the following steps
to move the pointed-to object:

- The object is move constructed at its new storage location
- The old object is destroyed, the type index is changed to the
  special `gc_moved` type index and its new position is recorded at the
  old location
- All tracked pointers internal to the object are added to the stack
- If the object has a `fixup` method it's called, which ensures that the
  untracked pointers are also added to the processing stack (this is
  where it's convenient that untracked pointers have the same
  representation as tracked pointers except for the `gc_heap` pointer)

After the object has been moved the pointer is set to to the objects
new location and the next pointer on the stack is processed.

Changing the type index of moved objects is crucial in avoiding infinite
recursion when processing self-references. When a pointer to a
`gc_moved` object is encountered the above steps are skipped and the
pointer is updated directly.

As long as all objects are implemented correctly, the above steps
ensures precise garbage collection and that all tracked and untracked
pointers are updated.

## Pitfalls

Even without considering the unsafe optimizations using the garbage
collector still requires extra care for both class implementers and
users alike compared to using `std::shared_ptr`.

The fact that objects can be moved around in memory means that the user
has to very careful about only keeping direct (real) pointers to objects
when they know a garbage collection can't happen.

This is easier said than done and was the cause of quote a few bugs.

From obvious cases where I was storing a reference to an object on the
GC heap: no go because objects can move.

Through the global object implementation capturing `this` in a
lambda: no go because `this` can move when you're an object on the GC heap!

To headscratchers like:

```C++
o->put(key, eval(some_expression));
```

not working because `o::operator->` may be evaluated before the call to
`eval` which may trigger a garbage collection, which may move `o`
making the result of `o->` stale!

### Usage Guidelines

During the development I've formulated some rules I try to follow.
They're probably not sufficient to avoid all errors but they're
necessary:

- Never keep raw references to GC objects - use managed pointers
  (tracker or untracked), as objects may move around in memory
- Be careful about the (sometimes very short) scope where a dereferenced
  heap pointer is valid
- Any class that might be constructed on the GC heap must not keep tracked
  pointers outside the GC heap. An example is having a
  `std::vector<gc_heap_ptr<...>>` as the pointers register as root
  pointers, but are only be destructed once the object is finalized,
  which means the garbage collector needs to run twice to clean up the
  mess
- In general be prepared for the destructor being called a lot later
  than normal (finalization)
- Be prepared to be moved and be inquired about your untracked pointers

## Future Work

While the garbage collector works there are still lots left to be
improved and explored.  Again I'll refer to my [TODO
list](https://github.com/mras0/mjs/blob/master/TODO.md) to see some of
the things I'm considering. Feel free to use the code as a starting
point or inspiration for your own experiments.

Not all of my requirements have been satisfied,
but I'm content for now and fairly confident that the code can be
adapted to future needs. I'm also not too keen on making more
invasive optimizations before I've decided on what kinds of workloads
I want to optimize for and what direction I want to take the code in.

## The Garbage Collector in Action

I've created a short [visualization](anim1/anim1.html) of
the garbage collector in action to maybe explain in pictures what I've
failed to make clear in text.  It shows most of the steps the garbage
collector takes when `h.garbage_collect()` is executed in the following
simplified example. It starts out showing all steps, but gradually skips
intermediary actions to not get too long.

```C++

// Simple gc-enabled class capable of holding a single (tracked) object reference
class test_obj {
public:
    explicit test_obj(const gc_heap_ptr<object>& p) : pointer(p) {}
private:
    gc_heap_ptr<object> pointer;
};

void vis() {
    // Create heap with 32 slots
    gc_heap h{32};

    // Leak a string (for show)
    string{h, "Leaked!"};

    // Create object "o"
    auto o = object::make(h,
        string{h, "Object"}, // Class name "Object"
        nullptr);            // No prototype

    // Set its [[Value]] to false
    o->internal_value(value{false});

    // Add a self-referencing property "K1"
    o->put(string{h, "K1"}, value{o});

    // And another property set to NaN
    o->put(string{h, "K2"}, value{NAN});

    // Create a "test_obj" instance referencing "o"
    auto c = h.make<test_obj>(o);

    // Collect garbage (see visualization)
    // There are two root pointers: to "o" (@0008) and "c" (@001a)
    h.garbage_collect();
}

```

For kicks I also created a squashed (and totally unreadable)
[video](anim2.webm) of the garbage collector running after interpreting
an empty statement. This means it's mostly the `global` object being
moved.


