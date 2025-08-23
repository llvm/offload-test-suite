[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          uint counter0 = 0;
          while ((counter0 < 3)) {
            counter0 = (counter0 + 1);
            if ((WaveGetLaneIndex() < 5)) {
              result = (result + WaveActiveSum(result));
            }
            if ((counter0 == 2)) {
              break;
            }
          }
          break;
        }
      case 2: {
          if ((WaveGetLaneIndex() == 11)) {
            if ((WaveGetLaneIndex() == 5)) {
              result = (result + WaveActiveMin(result));
            }
          }
          break;
        }
      }
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  }
}
