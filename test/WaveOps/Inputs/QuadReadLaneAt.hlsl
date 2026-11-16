// Checks scalars and vectors from all four source lanes in two quads.
// Each output component checks one width, from scalar to four components.
// Signed and unsigned integer runs use the same input bits.
typedef vector<TYPE, 4> Value4;

StructuredBuffer<Value4> In : register(t0);
RWStructuredBuffer<uint4> Out : register(u1);

uint4 check(Value4 Value, Value4 Expected, uint Lane) {
  TYPE Scalar = QuadReadLaneAt(Value.x, Lane);
  vector<TYPE, 2> Vec2 = QuadReadLaneAt(Value.xy, Lane);
  vector<TYPE, 3> Vec3 = QuadReadLaneAt(Value.xyz, Lane);
  Value4 Vec4 = QuadReadLaneAt(Value, Lane);
  return uint4(Scalar == Expected.x, all(Vec2 == Expected.xy),
               all(Vec3 == Expected.xyz), all(Vec4 == Expected));
}

[numthreads(8, 1, 1)] void main(uint3 DTID : SV_DispatchThreadID) {
  uint Index = DTID.x;
  uint InputIndex = WaveGetLaneIndex() % 8;
  uint Base = InputIndex & ~3u;
  Value4 Value = In[InputIndex];
  for (uint Lane = 0; Lane < 4; ++Lane) {
    Out[Index * 4 + Lane] = check(Value, In[Base + Lane], Lane);
  }
}
