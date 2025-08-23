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
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
    }
  case 1: {
      if ((WaveGetLaneIndex() == 0)) {
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
        }
        if ((WaveGetLaneIndex() == 6)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 4))) {
        result = (result + WaveActiveMax(result));
      }
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveSum(result));
        }
        if ((i0 == 2)) {
          break;
        }
      }
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum(7));
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(5));
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
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        uint counter3 = 0;
        while ((counter3 < 3)) {
          counter3 = (counter3 + 1);
          if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11))) {
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
            }
            if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveMin(result));
            }
          } else {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 2))) {
            result = (result + WaveActiveSum(result));
          }
          if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((WaveGetLaneIndex() == 8)) {
        result = (result + WaveActiveMax(8));
      }
    }
    break;
  }
  }
}
