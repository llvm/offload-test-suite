[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
    if ((WaveGetLaneIndex() >= 14)) {
      if ((WaveGetLaneIndex() >= 12)) {
        result = (result + WaveActiveMax(result));
      }
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        if ((WaveGetLaneIndex() == 1)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        if ((i0 == 1)) {
          break;
        }
      }
      if ((WaveGetLaneIndex() >= 12)) {
        result = (result + WaveActiveMin(result));
      }
    }
  } else {
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
    result = (result + WaveActiveMin(result));
  }
  for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
      result = (result + WaveActiveSum(4));
    }
    for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMax(result));
      }
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
        }
      case 1: {
          if (((WaveGetLaneIndex() % 2) == 0)) {
            result = (result + WaveActiveSum(2));
          }
          break;
        }
      }
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMax(result));
      }
      if ((i3 == 1)) {
        continue;
      }
    }
    if ((i2 == 1)) {
      break;
    }
  }
  }
  if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      uint counter4 = 0;
      while ((counter4 < 2)) {
        counter4 = (counter4 + 1);
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
      }
    }
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
      result = (result + WaveActiveMax(result));
    }
  } else {
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(result));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
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
      if ((WaveGetLaneIndex() >= 11)) {
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveSum(9));
        }
        if ((WaveGetLaneIndex() < 6)) {
          result = (result + WaveActiveMin(3));
        }
      } else {
      if ((WaveGetLaneIndex() < 1)) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  }
  if ((WaveGetLaneIndex() == 10)) {
    result = (result + WaveActiveMin(result));
  }
  }
  for (uint i5 = 0; (i5 < 2); i5 = (i5 + 1)) {
    for (uint i6 = 0; (i6 < 2); i6 = (i6 + 1)) {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
            if ((WaveGetLaneIndex() < 7)) {
              result = (result + WaveActiveMin(result));
            }
            if ((WaveGetLaneIndex() >= 13)) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if ((i7 == 1)) {
              break;
            }
          }
        }
      case 1: {
          for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
            if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 7))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
            if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 9))) {
              result = (result + WaveActiveSum(result));
            }
          }
        }
      case 2: {
          uint counter9 = 0;
          while ((counter9 < 2)) {
            counter9 = (counter9 + 1);
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveMax(result));
            }
          }
          break;
        }
      }
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
      result = (result + WaveActiveSum(result));
    }
  }
}
