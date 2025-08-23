[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() < 4)) {
    if ((WaveGetLaneIndex() >= 14)) {
      result = (result + WaveActiveSum(result));
    }
    if ((WaveGetLaneIndex() >= 13)) {
      result = (result + WaveActiveSum(result));
    }
  }
}
