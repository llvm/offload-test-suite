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
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  case 2: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          switch ((WaveGetLaneIndex() % 3)) {
          case 0: {
              if ((WaveGetLaneIndex() >= 12)) {
                if ((WaveGetLaneIndex() >= 12)) {
                  result = (result + WaveActiveMin(8));
                }
              } else {
              if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 7))) {
                result = (result + WaveActiveSum(WaveGetLaneIndex()));
              }
              if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 3))) {
                result = (result + WaveActiveMin(WaveGetLaneIndex()));
              }
            }
            break;
          }
        case 1: {
            uint counter0 = 0;
            while ((counter0 < 2)) {
              counter0 = (counter0 + 1);
              if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
                result = (result + WaveActiveSum(3));
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
        }
        break;
      }
    case 1: {
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
              if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
                result = (result + WaveActiveMax(result));
              }
              if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
                result = (result + WaveActiveSum(2));
              }
            }
            break;
          }
        case 1: {
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
              if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
                result = (result + WaveActiveMin(WaveGetLaneIndex()));
              }
            } else {
            if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13))) {
              result = (result + WaveActiveMin(result));
            }
          }
          break;
        }
      case 2: {
          for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveMax(result));
            }
          }
          break;
        }
      }
      break;
    }
  case 2: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
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
      } else {
      for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
        }
        if ((i3 == 1)) {
          continue;
        }
        if ((i3 == 1)) {
          break;
        }
      }
      if ((WaveGetLaneIndex() == 10)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
      }
    }
    break;
  }
  }
  break;
  }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      uint counter4 = 0;
      while ((counter4 < 2)) {
        counter4 = (counter4 + 1);
        if ((WaveGetLaneIndex() < 6)) {
          if ((WaveGetLaneIndex() >= 10)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
        } else {
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((WaveGetLaneIndex() >= 10)) {
        result = (result + WaveActiveSum(3));
      }
    }
    break;
  }
  case 1: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if ((WaveGetLaneIndex() < 4)) {
          if ((WaveGetLaneIndex() >= 14)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() >= 12)) {
            result = (result + WaveActiveMax(result));
          }
        }
        break;
      }
    case 1: {
        for (uint i5 = 0; (i5 < 2); i5 = (i5 + 1)) {
          if ((WaveGetLaneIndex() == 7)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
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
    }
    break;
  }
  }
}
