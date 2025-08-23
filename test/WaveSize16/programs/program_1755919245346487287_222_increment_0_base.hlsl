[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 5))) {
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMin(result));
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 3))) {
          for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
            if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveSum(result));
            }
            if ((i0 == 1)) {
              continue;
            }
            if ((i0 == 2)) {
              break;
            }
          }
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveSum(result));
          }
        }
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMax(result));
      }
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
        }
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
          }
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveSum(result));
          }
        } else {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      }
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMax(4));
    }
  }
  break;
  }
  case 1: {
    uint counter2 = 0;
    while ((counter2 < 3)) {
      counter2 = (counter2 + 1);
      if ((WaveGetLaneIndex() < 3)) {
        result = (result + WaveActiveSum(result));
      }
      uint counter3 = 0;
      while ((counter3 < 3)) {
        counter3 = (counter3 + 1);
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
        }
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 3))) {
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
          }
          if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveMax(1));
          }
        }
        if ((counter3 == 2)) {
          break;
        }
      }
      if ((WaveGetLaneIndex() < 5)) {
        result = (result + WaveActiveMin(10));
      }
      if ((counter2 == 2)) {
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
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 5))) {
        uint counter4 = 0;
        while ((counter4 < 3)) {
          counter4 = (counter4 + 1);
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 3))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
          }
        }
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 0))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
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
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10))) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMax(result));
        }
        for (uint i5 = 0; (i5 < 2); i5 = (i5 + 1)) {
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveSum(result));
          }
          if ((i5 == 1)) {
            break;
          }
        }
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin(result));
        }
      }
    }
  case 3: {
      if ((WaveGetLaneIndex() < 20)) {
        result = (result + WaveActiveSum(4));
      }
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
        for (uint i6 = 0; (i6 < 3); i6 = (i6 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(result));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if ((i6 == 1)) {
            continue;
          }
        }
        if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveSum(result));
        }
      } else {
      if ((WaveGetLaneIndex() < 2)) {
        result = (result + WaveActiveMax(5));
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
    }
    break;
  }
  case 1: {
    uint counter7 = 0;
    while ((counter7 < 3)) {
      counter7 = (counter7 + 1);
      if ((WaveGetLaneIndex() < 4)) {
        result = (result + WaveActiveMax(5));
      }
      if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 10))) {
        if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveSum(result));
        }
      }
      if ((counter7 == 2)) {
        break;
      }
    }
    break;
  }
  case 2: {
    for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
        result = (result + WaveActiveSum(6));
      }
      uint counter9 = 0;
      while ((counter9 < 3)) {
        counter9 = (counter9 + 1);
        if ((WaveGetLaneIndex() >= 11)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
        }
      }
    }
    break;
  }
  }
}
