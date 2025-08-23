[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() < 4)) {
    if ((WaveGetLaneIndex() >= 10)) {
      result = (result + WaveActiveMin(2));
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
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10))) {
              if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 12))) {
                result = (result + WaveActiveMax(WaveGetLaneIndex()));
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
            if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 2))) {
              if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 11))) {
                result = (result + WaveActiveMin(result));
              }
              if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
                result = (result + WaveActiveMin(result));
              }
            }
            break;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(8));
        }
      } else {
      if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 13))) {
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
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() == 14)) {
            if ((WaveGetLaneIndex() == 2)) {
              result = (result + WaveActiveSum(result));
            }
            if ((WaveGetLaneIndex() == 10)) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          uint counter0 = 0;
          while ((counter0 < 2)) {
            counter0 = (counter0 + 1);
            if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 7))) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
            }
          }
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        break;
      }
    case 1: {
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
            break;
          }
        case 1: {
            for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
              if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
                result = (result + WaveActiveSum(result));
              }
              if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
                result = (result + WaveActiveMax(result));
              }
            }
            break;
          }
        case 2: {
            for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
              if ((WaveGetLaneIndex() == 6)) {
                result = (result + WaveActiveMax(result));
              }
              if ((i2 == 2)) {
                break;
              }
            }
            break;
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
        break;
      }
    }
    break;
  }
  case 1: {
    if ((WaveGetLaneIndex() >= 15)) {
      uint counter3 = 0;
      while ((counter3 < 2)) {
        counter3 = (counter3 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((i4 == 2)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(8));
        }
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
}
