[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
      result = (result + WaveActiveMax(result));
    }
    switch ((WaveGetLaneIndex() % 4)) {
    case 0: {
        for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() == 12)) {
            result = (result + WaveActiveSum(result));
          }
          if ((i0 == 1)) {
            continue;
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
        if ((WaveGetLaneIndex() < 5)) {
          if ((WaveGetLaneIndex() >= 13)) {
            result = (result + WaveActiveSum(result));
          }
          for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMax(result));
            }
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
            if ((i1 == 2)) {
              break;
            }
          }
          if ((WaveGetLaneIndex() >= 8)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        } else {
        if ((WaveGetLaneIndex() == 0)) {
          result = (result + WaveActiveMax(result));
        }
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
          if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
          }
        }
        if ((WaveGetLaneIndex() == 14)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 3: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15))) {
            if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
            }
          } else {
          if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveSum(result));
          }
          if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 10))) {
            result = (result + WaveActiveSum(result));
          }
        }
        break;
      }
    case 2: {
        for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 1))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
          }
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 7))) {
            result = (result + WaveActiveSum(result));
          }
          if ((i3 == 1)) {
            break;
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
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMax(result));
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(result));
    }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            if ((WaveGetLaneIndex() < 7)) {
              if ((WaveGetLaneIndex() < 1)) {
                result = (result + WaveActiveSum(result));
              }
            } else {
            if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
              result = (result + WaveActiveMin(result));
            }
          }
        }
      case 1: {
          uint counter4 = 0;
          while ((counter4 < 2)) {
            counter4 = (counter4 + 1);
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveSum(result));
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
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(result));
      }
    } else {
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
      result = (result + WaveActiveSum(2));
    }
    for (uint i5 = 0; (i5 < 2); i5 = (i5 + 1)) {
      for (uint i6 = 0; (i6 < 3); i6 = (i6 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      if ((i5 == 1)) {
        continue;
      }
      if ((i5 == 1)) {
        break;
      }
    }
  }
  break;
  }
  case 1: {
    for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMin(result));
      }
      uint counter8 = 0;
      while ((counter8 < 3)) {
        counter8 = (counter8 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
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
        }
      }
      if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8))) {
        result = (result + WaveActiveMax(result));
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
  case 3: {
    for (uint i9 = 0; (i9 < 2); i9 = (i9 + 1)) {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 6))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
        for (uint i10 = 0; (i10 < 2); i10 = (i10 + 1)) {
          if ((WaveGetLaneIndex() == 1)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
      } else {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveMax(6));
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
      }
      if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
      result = (result + WaveActiveMax(result));
    }
    if ((i9 == 1)) {
      break;
    }
  }
  break;
  }
  }
}
