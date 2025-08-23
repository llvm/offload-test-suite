[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() == 2)) {
        if ((WaveGetLaneIndex() == 5)) {
          result = (result + WaveActiveMax(result));
        }
        for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
          uint counter1 = 0;
          while ((counter1 < 3)) {
            counter1 = (counter1 + 1);
            if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveMax(result));
            }
            if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if ((counter1 == 2)) {
              break;
            }
          }
        }
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
}
