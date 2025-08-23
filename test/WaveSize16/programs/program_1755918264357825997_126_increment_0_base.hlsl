[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() == 2)) {
    if ((WaveGetLaneIndex() == 11)) {
      result = (result + WaveActiveMin(result));
    }
    if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 10))) {
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMin(result));
      }
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() % 2) == 0)) {
            result = (result + WaveActiveSum(2));
          }
          break;
        }
      case 2: {
          if (true) {
            result = (result + WaveActiveSum(3));
          }
          break;
        }
      case 3: {
          if ((WaveGetLaneIndex() < 20)) {
            result = (result + WaveActiveSum(4));
          }
          break;
        }
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveSum(result));
      }
    }
    if ((WaveGetLaneIndex() == 7)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
    }
  }
}
