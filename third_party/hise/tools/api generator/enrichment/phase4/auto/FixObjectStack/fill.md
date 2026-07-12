Copies the data from the given object into every allocated slot across the full `length` capacity.

> [!Warning:Does not update used count] Does not update the used count. After `fill()`, `size()` still returns its previous value even though all slots now contain valid data.