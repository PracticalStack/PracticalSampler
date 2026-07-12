# Engine Adapter

This directory is reserved for the product-owned integration boundary between Decent Rhapsody Studio and HISE.

Only code in this layer should directly mediate access to HISE-backed runtime services. Product domain models and higher-level app features should depend on stable adapter interfaces rather than on HISE internals.
