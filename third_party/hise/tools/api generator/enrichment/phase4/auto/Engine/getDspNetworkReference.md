Returns a reference to a DSP network owned by another script processor, enabling cross-processor access to scriptnode networks. Pass the processor ID and the network name.

> [!Warning:Wrong processor ID returns undefined] If the processor ID does not match any scriptnode-enabled module, the method silently returns `undefined`. Only a wrong network ID on a found processor produces an error.