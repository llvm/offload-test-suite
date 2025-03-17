# Using Root Signatures (DirectX-only)

## Auto-generated Root Signatures

By default the test suite will use a root signature if it is present in the
shader binary. If the shader binary does not contain a precompiled root
signature, a root signature is generated from the DescriptorSets and the
`DXBinding` YAML data.

For example the YAML below:

```yaml
DescriptorSets:
  - Resources:
    - Name: In
      Kind: StructuredBuffer
      DirectXBinding:
        Register: 0
        Space: 0
    - Name: Out1
      Kind: RWBuffer
      DirectXBinding:
        Register: 1
        Space: 4
  - Resources:
    - Name: Out2
      Kind: RWBuffer
      DirectXBinding:
        Register: 2
        Space: 4
```

Generates a root signature rougly equivalent to `DescriptorTable(SRV(t0),
UAV(u1, space=4)), DescriptorTable(u2, space=4)`

This mechanism has some limitations, specifically it does not support root
descriptors or root constants.

## Explicit Root Signatures

For more complicated root signatures a root parameter list may be specified as
part of the pipeline's `RuntimeSettings`. Explicit root parameter lists can be
used to specify root constants, descriptor tables, and root descriptors in
arbitrary order.

1) An explicit root parameter list must contain as many `DescriptorTable` root
   parameters as the pipeline has elements in the `DescriptorSet` array.
2) The order of root descriptor tables will match the order of `DescriptorSet`
   entries.

### Specifying Root Descriptor Tables

Root descriptor tables have no required additonal information, they just specify
the parameter kind:

```yaml
- Kind: DescriptorTable
```

### Specifying Root Constants

A root constant is specified in the YAML by providing the kind `Constant` and
name of the buffer to map to the constant in YAML:

```yaml
- Kind: Constant
  Name: Root
```

### Specifying Root Descriptors

A root descriptor is specified in YAML by providing the resource declaration with a name and resource type:

```yaml
- Kind: RootDescriptor
  Resource:
    Name: cbuffer
    Kind: ConstantBuffer
- Kind: RootDescriptor
  Resource:
    Name: In
    Kind: StructuredBuffer
- Kind: RootDescriptor
  Resource:
    Name: Out
    Kind: RWStructuredBuffer
```

> Note: Root descriptors must be raw buffer types as required by DirectX. The
> YAML parser will emit an error if the descriptors are incorrect types.