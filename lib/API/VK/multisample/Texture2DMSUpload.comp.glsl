#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, rgba32f) uniform image2DMS dstImage;

layout(std430, binding = 1) readonly buffer SrcBuffer {
  vec4 data[];
} src;

layout(push_constant) uniform PushConstants {
  uint Width;
  uint Height;
  uint SampleCount;
} pc;

void main() {
  uint x = gl_GlobalInvocationID.x;
  uint y = gl_GlobalInvocationID.y;
  uint sampleIndex = gl_GlobalInvocationID.z;
  if (x >= pc.Width || y >= pc.Height || sampleIndex >= pc.SampleCount)
    return;

  uint index = ((y * pc.Width) + x) * pc.SampleCount + sampleIndex;
  imageStore(dstImage, ivec2(int(x), int(y)), int(sampleIndex),
             src.data[index]);
}