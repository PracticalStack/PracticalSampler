FixObjectArray (object)
Obtain via: FixObjectFactory.createArray(numElements)

Fixed-size array of typed objects with contiguous memory layout. All slots are
always valid -- bracket indexing, for-in loops, and search methods operate over
the full capacity. No allocations after creation, suitable for real-time contexts.

Constants:
  Size:
    length = (constructor arg)    Fixed number of elements in the array

Complexity tiers:
  1. Object pool with iteration: bracket indexing, for-in loop. Fixed number of
     typed records with per-frame updates.
  2. Column extraction pipeline: + copy. Extract individual properties into
     Buffers for downstream DSP or shader consumption.
  3. Persistent state: + toBase64, fromBase64. Save and restore array state
     across sessions (binary layout-dependent).

Practical defaults:
  - Use FixObjectArray when all slots are always meaningful (e.g., a particle
    pool where inactive ones have gain = 0). Use FixObjectStack when elements
    are dynamically inserted and removed.
  - Set the compare function on the factory before creating arrays if you plan
    to use indexOf, contains, or sort. The default byte-level comparator is
    rarely what you want.
  - Match Buffer sizes to the array's length constant when using copy(). A size
    mismatch produces a script error.

Common mistakes:
  - Calling sort() without setting a compare function on the factory first --
    produces meaningless ordering (pointer address comparison), no warning.
  - Assuming fromBase64() throws on size mismatch -- it silently returns 0.
    Always check the return value.
  - Creating Buffers with a different size than the array for copy() -- produces
    a script error. Use Buffer.create(array.length).
  - Using a regular Array for real-time visual state tracking -- Array allocates
    on modification. FixObjectArray's pre-allocated memory is safe for timer
    callbacks.

Example:
  // Create a factory and array
  const var f = Engine.createFixObjectFactory({
      "id": 0,
      "velocity": 0.0,
      "active": false
  });

  const var a = f.createArray(128);

  // Access elements via bracket indexing
  a[0].id = 42;
  a[0].velocity = 0.8;

  // Iterate with for-in
  for (obj in a)
      obj.active = true;

Methods (9):
  clear       contains    copy
  fill        fromBase64  indexOf
  size        sort        toBase64
