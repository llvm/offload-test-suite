[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() >= 8)) {
        switch ((WaveGetLaneIndex() % 4)) {
        case 0: {
            uint counter0 = 0;
            while ((counter0 < 2)) {
              counter0 = (counter0 + 1);
              if ((WaveGetLaneIndex() == 4)) {
                result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
              }
            }
          }
        case 1: {
            if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 9))) {
              if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 14))) {
                result = (result + WaveActiveMax(4));
              }
            }
          }
        case 2: {
            uint counter1 = 0;
            while ((counter1 < 3)) {
              counter1 = (counter1 + 1);
              if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
                result = (result + WaveActiveMax(WaveGetLaneIndex()));
              }
              if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
                result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
              }
            }
          }
        case 3: {
            uint counter2 = 0;
            while ((counter2 < 3)) {
              counter2 = (counter2 + 1);
              if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 8))) {
                result = (result + WaveActiveMax(result));
              }
            }
            break;
          }
        }
        if ((WaveGetLaneIndex() < 4)) {
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
      uint counter3 = 0;
      while ((counter3 < 3)) {
        counter3 = (counter3 + 1);
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveMax(9));
          }
          uint counter4 = 0;
          while ((counter4 < 2)) {
            counter4 = (counter4 + 1);
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMax(result));
            }
            if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10))) {
              result = (result + WaveActiveMin(result));
            }
            if ((counter4 == 1)) {
              break;
            }
          }
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveMin(result));
          }
        } else {
        if ((WaveGetLaneIndex() == 3)) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() == 7)) {
          result = (result + WaveActiveMin(result));
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
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
    }
  case 2: {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
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
        default: {
            result = (result + WaveActiveSum(99));
            break;
          }
        }
        if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  }
}
