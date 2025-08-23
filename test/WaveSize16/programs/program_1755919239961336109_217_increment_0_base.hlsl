[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin(9));
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  }
}
