[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
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
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
            if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMax(result));
            }
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
          break;
        }
      }
      break;
    }
  case 2: {
      if ((WaveGetLaneIndex() == 2)) {
        if ((WaveGetLaneIndex() == 10)) {
          result = (result + WaveActiveMax(7));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          }
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveMax(result));
          }
          if ((i1 == 2)) {
            break;
          }
        }
      } else {
      if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
      }
      switch ((WaveGetLaneIndex() % 2)) {
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
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  case 2: {
      if ((WaveGetLaneIndex() == 12)) {
        if ((WaveGetLaneIndex() == 0)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if ((WaveGetLaneIndex() < 4)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum(result));
            }
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
          }
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(result));
          }
        }
        if ((WaveGetLaneIndex() == 9)) {
          result = (result + WaveActiveMin(5));
        }
      }
      break;
    }
  }
}
