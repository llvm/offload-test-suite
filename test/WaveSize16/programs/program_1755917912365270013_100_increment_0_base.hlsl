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
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if ((i1 == 1)) {
              continue;
            }
            if ((i1 == 1)) {
              break;
            }
          }
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveSum(result));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        }
      }
      break;
    }
  case 2: {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(result));
        }
        if ((WaveGetLaneIndex() >= 8)) {
          if ((WaveGetLaneIndex() < 3)) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() >= 9)) {
            if ((WaveGetLaneIndex() < 7)) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
          }
          if ((WaveGetLaneIndex() < 5)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax(4));
        }
      } else {
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 2))) {
        result = (result + WaveActiveMin(result));
      }
      for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 11))) {
          if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax(5));
          }
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMax(3));
          }
        } else {
        if ((WaveGetLaneIndex() >= 12)) {
          result = (result + WaveActiveMin(result));
        }
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      if ((i2 == 1)) {
        continue;
      }
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
  }
  uint counter3 = 0;
  while ((counter3 < 3)) {
    counter3 = (counter3 + 1);
    if ((WaveGetLaneIndex() >= 8)) {
      if ((WaveGetLaneIndex() < 7)) {
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
    if ((WaveGetLaneIndex() < 1)) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
    }
  }
  if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 1))) {
    if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 8))) {
      result = (result + WaveActiveMin(3));
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(1));
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
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(7));
      }
    }
    if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 1))) {
      result = (result + WaveActiveSum(result));
    }
  } else {
  if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 7))) {
    result = (result + WaveActiveMax(7));
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMax(result));
    }
    if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 1))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax(6));
    }
  }
  }
}
