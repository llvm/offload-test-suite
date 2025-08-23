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
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveMin(result));
        }
        if ((WaveGetLaneIndex() >= 11)) {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() >= 12)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveSum(result));
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
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 3))) {
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
        }
        switch ((WaveGetLaneIndex() % 2)) {
        case 0: {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveMax(result));
              }
              if (((WaveGetLaneIndex() & 1) == 1)) {
                result = (result + WaveActiveSum(result));
              }
            } else {
            if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 4))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
            }
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
          } else {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
          }
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        break;
      }
    default: {
        result = (result + WaveActiveSum(99));
        break;
      }
    }
  } else {
  for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
    if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMin(result));
    }
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
      }
    }
    if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 10))) {
      result = (result + WaveActiveSum(result));
    }
    if ((i1 == 1)) {
      break;
    }
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
    for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
        result = (result + WaveActiveSum(10));
      }
      if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 0))) {
        if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 6))) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMax(result));
          }
          if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 4))) {
            result = (result + WaveActiveMax(10));
          }
        }
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveMax(2));
        }
      }
    }
    break;
  }
  case 3: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        uint counter4 = 0;
        while ((counter4 < 2)) {
          counter4 = (counter4 + 1);
          if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
          }
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 9))) {
            if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 4))) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 0))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveMax(3));
        }
      }
      break;
    }
  case 1: {
      uint counter5 = 0;
      while ((counter5 < 2)) {
        counter5 = (counter5 + 1);
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveSum(2));
        }
        uint counter6 = 0;
        while ((counter6 < 2)) {
          counter6 = (counter6 + 1);
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
          }
          if ((counter6 == 1)) {
            break;
          }
        }
      }
      break;
    }
  case 2: {
      if ((WaveGetLaneIndex() == 3)) {
        if ((WaveGetLaneIndex() == 14)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
          if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 11))) {
            result = (result + WaveActiveMax(10));
          }
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
        }
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
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
    }
  case 1: {
      for (uint i7 = 0; (i7 < 3); i7 = (i7 + 1)) {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() == 10)) {
            if ((WaveGetLaneIndex() == 9)) {
              result = (result + WaveActiveMax(result));
            }
            if ((WaveGetLaneIndex() == 9)) {
              result = (result + WaveActiveMax(8));
            }
          } else {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
        }
        if ((i8 == 1)) {
          break;
        }
      }
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveSum(result));
      }
      if ((i7 == 1)) {
        continue;
      }
      if ((i7 == 2)) {
        break;
      }
    }
    break;
  }
  }
}
