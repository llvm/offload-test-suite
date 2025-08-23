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
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  case 3: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
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
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
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
      break;
    }
  case 2: {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 7))) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum(result));
        }
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveSum(WaveGetLaneIndex()));
              }
              if (((WaveGetLaneIndex() & 1) == 1)) {
                result = (result + WaveActiveMin(result));
              }
            }
            break;
          }
        case 1: {
            if ((WaveGetLaneIndex() == 0)) {
              if ((WaveGetLaneIndex() == 13)) {
                result = (result + WaveActiveMax(WaveGetLaneIndex()));
              }
            } else {
            if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveMin(result));
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
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMax(result));
      }
    } else {
    if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 5))) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        if ((WaveGetLaneIndex() >= 8)) {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveMin(result));
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
    if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 10))) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
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
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            if ((WaveGetLaneIndex() == 9)) {
              if ((WaveGetLaneIndex() == 13)) {
                result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
              }
              if ((WaveGetLaneIndex() == 11)) {
                result = (result + WaveActiveMax(2));
              }
            }
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if ((WaveGetLaneIndex() < 4)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12))) {
            if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11))) {
              result = (result + WaveActiveMin(result));
            }
            if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMax(result));
            }
          }
          if ((WaveGetLaneIndex() < 2)) {
            result = (result + WaveActiveMin(result));
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
    for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
        result = (result + WaveActiveSum(result));
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
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
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(9));
        }
      }
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
        if (true) {
          result = (result + WaveActiveSum(3));
        }
        break;
      }
    }
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
