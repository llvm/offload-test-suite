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
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  case 2: {
      if ((WaveGetLaneIndex() < 7)) {
        if ((WaveGetLaneIndex() < 5)) {
          result = (result + WaveActiveMin(result));
        }
        switch ((WaveGetLaneIndex() % 2)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
            break;
          }
        case 1: {
            for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
              if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
                result = (result + WaveActiveMin(9));
              }
              if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 2))) {
                result = (result + WaveActiveMin(WaveGetLaneIndex()));
              }
            }
            break;
          }
        default: {
            result = (result + WaveActiveSum(99));
            break;
          }
        }
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
      } else {
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if ((WaveGetLaneIndex() >= 15)) {
          result = (result + WaveActiveMin(4));
        }
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
          if ((WaveGetLaneIndex() < 6)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
          if ((WaveGetLaneIndex() < 3)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
        }
        if ((WaveGetLaneIndex() < 3)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
        if ((counter1 == 1)) {
          break;
        }
      }
      if ((WaveGetLaneIndex() >= 8)) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  case 3: {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveSum(result));
      }
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          uint counter3 = 0;
          while ((counter3 < 3)) {
            counter3 = (counter3 + 1);
            if ((WaveGetLaneIndex() == 8)) {
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
      }
    } else {
    uint counter4 = 0;
    while ((counter4 < 3)) {
      counter4 = (counter4 + 1);
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      if ((counter4 == 2)) {
        break;
      }
    }
  }
  break;
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
        if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
        }
        uint counter6 = 0;
        while ((counter6 < 2)) {
          counter6 = (counter6 + 1);
          if ((WaveGetLaneIndex() == 12)) {
            result = (result + WaveActiveMax(7));
          }
        }
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveSum(result));
        }
      }
    }
  case 1: {
      if ((WaveGetLaneIndex() == 2)) {
        if ((WaveGetLaneIndex() == 7)) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 1))) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMin(result));
        }
        switch ((WaveGetLaneIndex() % 4)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
          }
        case 1: {
            if (((WaveGetLaneIndex() % 2) == 0)) {
              result = (result + WaveActiveSum(2));
            }
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
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
      }
    }
    break;
  }
  case 2: {
    if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 2))) {
      for (uint i7 = 0; (i7 < 3); i7 = (i7 + 1)) {
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(result));
          }
          if ((i8 == 1)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        }
        if ((i7 == 1)) {
          continue;
        }
      }
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 1))) {
        result = (result + WaveActiveMax(8));
      }
    } else {
    uint counter9 = 0;
    while ((counter9 < 2)) {
      counter9 = (counter9 + 1);
      if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 2))) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 5))) {
          result = (result + WaveActiveMin(result));
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMax(result));
        }
      }
    }
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
      result = (result + WaveActiveMin(5));
    }
  }
  }
  case 3: {
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
        result = (result + WaveActiveMax(3));
      }
      for (uint i10 = 0; (i10 < 2); i10 = (i10 + 1)) {
        if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 5))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
        uint counter11 = 0;
        while ((counter11 < 3)) {
          counter11 = (counter11 + 1);
          if ((WaveGetLaneIndex() < 6)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
          }
          if ((WaveGetLaneIndex() >= 12)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
      }
    } else {
    for (uint i12 = 0; (i12 < 2); i12 = (i12 + 1)) {
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
      }
      if ((WaveGetLaneIndex() == 7)) {
        if ((WaveGetLaneIndex() == 12)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      }
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
        result = (result + WaveActiveMin(result));
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMin(result));
    }
  }
  break;
  }
  }
}
