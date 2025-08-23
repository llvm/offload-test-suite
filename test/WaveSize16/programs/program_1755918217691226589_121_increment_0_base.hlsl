[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
        }
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
          if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveMax(result));
      }
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
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveSum(1));
      }
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
  }
}
