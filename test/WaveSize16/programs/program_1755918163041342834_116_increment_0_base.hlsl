[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
            if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveMax(result));
            }
            if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13))) {
              result = (result + WaveActiveSum(result));
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
      case 2: {
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
            if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 11))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
            }
            if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
            }
          }
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
  case 2: {
      switch ((WaveGetLaneIndex() % 3)) {
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
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
}
