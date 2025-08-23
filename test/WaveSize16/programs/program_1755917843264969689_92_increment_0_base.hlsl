[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
      result = (result + WaveActiveSum(result));
    }
  }
}
