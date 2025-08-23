[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() == 3)) {
    uint counter0 = 0;
    while ((counter0 < 3)) {
      counter0 = (counter0 + 1);
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum(result));
        }
      } else {
      if ((WaveGetLaneIndex() == 7)) {
        result = (result + WaveActiveMax(result));
      }
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
  }
  if ((WaveGetLaneIndex() == 6)) {
    result = (result + WaveActiveMin(result));
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
            if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 10))) {
              result = (result + WaveActiveMin(result));
            }
            uint counter2 = 0;
            while ((counter2 < 2)) {
              counter2 = (counter2 + 1);
              if ((WaveGetLaneIndex() < 8)) {
                result = (result + WaveActiveMin(8));
              }
            }
            if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMax(result));
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
          if (true) {
            result = (result + WaveActiveSum(3));
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
