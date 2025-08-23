[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(result));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
        result = (result + WaveActiveMax(result));
      }
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
        if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 2))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
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
        if ((WaveGetLaneIndex() < 1)) {
          if ((WaveGetLaneIndex() >= 11)) {
            result = (result + WaveActiveMin(3));
          }
          if ((WaveGetLaneIndex() < 4)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
          }
        }
        break;
      }
    }
    break;
  }
  }
}
