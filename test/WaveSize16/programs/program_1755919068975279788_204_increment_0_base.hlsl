[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveSum(3));
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveSum(result));
          }
          if ((i1 == 2)) {
            break;
          }
        }
        if ((WaveGetLaneIndex() >= 13)) {
          result = (result + WaveActiveSum(result));
        }
        if ((i0 == 1)) {
          break;
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
