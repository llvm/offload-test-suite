#--- source.hlsl
RWStructuredBuffer<uint> _participant_check_sum : register(u1);
RWStructuredBuffer<uint> _participant_bit : register(u2);

[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  _participant_check_sum[tid.x] = WaveActiveSum(1);
  _participant_bit[tid.x] = WaveActiveMax(tid.x);
}

#--- pipeline.yaml
---
Shaders:
  - Stage: Compute
    Entry: main
    DispatchSize: [1, 1, 1]  # Single dispatch for 64 threads
Buffers:
  - Name: _participant_check_sum
    Format: UInt32
    Stride: 4
    Fill: 0
    Size: 64
  - Name: expected_participants
    Format: UInt32
    Stride: 4
    Data: [0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
  - Name: _participant_bit
    Format: UInt32
    Stride: 4
    Fill: 0
    Size: 512
  - Name: _index
    Format: UInt32
    Stride: 4
    Fill: 0
    Size: 4
Results:
  - Result: WaveOpValidation
    Rule: BufferExact
    Actual: _participant_check_sum
    Expected: expected_participants
  - Result: WaveOpParticipants
    Rule: BufferExact
    Actual: _participant_bit
    Expected: expected_participants
DescriptorSets:
  - Resources:
    - Name: _participant_check_sum
      Kind: RWStructuredBuffer
      DirectXBinding:
        Register: 1
        Space: 0
      VulkanBinding:
        Binding: 1
    - Name: _participant_bit
      Kind: RWStructuredBuffer
      DirectXBinding:
        Register: 2
        Space: 0
      VulkanBinding:
        Binding: 2
    - Name: _index
      Kind: RWStructuredBuffer
      DirectXBinding:
        Register: 3
        Space: 0
      VulkanBinding:
        Binding: 3
...
#--- end

# RUN: split-file %s %t
# RUN: %dxc_target -T cs_6_0 -Fo %t.o %t/source.hlsl
# RUN: %offloader %t/pipeline.yaml %t.o
