[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
    uint counter1 = 0;
    while ((counter1 < 3)) {
      counter1 = (counter1 + 1);
      if ((WaveGetLaneIndex() == 13)) {
        result = (result + WaveActiveMax(result));
      }
      if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 12))) {
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
        }
      }
      if ((WaveGetLaneIndex() == 10)) {
        result = (result + WaveActiveMin(result));
      }
      if ((counter1 == 2)) {
        break;
      }
    }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
          if ((WaveGetLaneIndex() >= 15)) {
            result = (result + WaveActiveMin(result));
          }
          if ((WaveGetLaneIndex() == 6)) {
            if ((WaveGetLaneIndex() == 13)) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
          }
        }
        if ((i3 == 2)) {
          break;
        }
      }
      if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  case 2: {
    if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14))) {
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMax(result));
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
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
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
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
        if ((WaveGetLaneIndex() >= 8)) {
          result = (result + WaveActiveSum(result));
        }
        if ((WaveGetLaneIndex() == 0)) {
          uint counter5 = 0;
          while ((counter5 < 3)) {
            counter5 = (counter5 + 1);
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum(result));
            }
          }
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveMax(result));
          }
        } else {
        if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 10))) {
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveMax(9));
        }
      }
      if ((i4 == 2)) {
        break;
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
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        for (uint i6 = 0; (i6 < 3); i6 = (i6 + 1)) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
          if ((WaveGetLaneIndex() == 14)) {
            if ((WaveGetLaneIndex() == 2)) {
              result = (result + WaveActiveSum(4));
            }
            if ((WaveGetLaneIndex() == 2)) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
          }
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
        }
      }
      break;
    }
  case 1: {
      uint counter7 = 0;
      while ((counter7 < 2)) {
        counter7 = (counter7 + 1);
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMin(2));
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
