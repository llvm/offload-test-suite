[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
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
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 8))) {
            if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveSum(result));
          }
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 4))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          }
        }
        break;
      }
    case 2: {
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(result));
          }
        }
        break;
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        if ((WaveGetLaneIndex() == 11)) {
          result = (result + WaveActiveMax(result));
        }
        uint counter3 = 0;
        while ((counter3 < 3)) {
          counter3 = (counter3 + 1);
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() == 4)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((WaveGetLaneIndex() == 2)) {
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
      uint counter4 = 0;
      while ((counter4 < 2)) {
        counter4 = (counter4 + 1);
        uint counter5 = 0;
        while ((counter5 < 2)) {
          counter5 = (counter5 + 1);
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
      }
      break;
    }
  case 3: {
      uint counter6 = 0;
      while ((counter6 < 2)) {
        counter6 = (counter6 + 1);
        if ((WaveGetLaneIndex() == 11)) {
          if ((WaveGetLaneIndex() == 2)) {
            result = (result + WaveActiveSum(6));
          }
          if ((WaveGetLaneIndex() == 15)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
          }
        }
        if ((WaveGetLaneIndex() == 14)) {
          result = (result + WaveActiveMax(result));
        }
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
  if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
    uint counter7 = 0;
    while ((counter7 < 2)) {
      counter7 = (counter7 + 1);
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveSum(result));
      }
      for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
        if ((WaveGetLaneIndex() == 6)) {
          result = (result + WaveActiveMax(9));
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 6))) {
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMax(result));
          }
        } else {
        if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
        }
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax(6));
        }
      }
      if ((WaveGetLaneIndex() == 15)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
    if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMax(result));
    }
  }
  if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
    result = (result + WaveActiveMin(WaveGetLaneIndex()));
  }
  } else {
  if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 15))) {
    result = (result + WaveActiveMax(9));
  }
  }
}
