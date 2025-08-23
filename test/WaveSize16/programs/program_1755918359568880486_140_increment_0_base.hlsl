[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
        }
        for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 1))) {
            result = (result + WaveActiveMin(result));
          }
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 10))) {
            if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
              result = (result + WaveActiveSum(5));
            }
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
        }
      } else {
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveSum(result));
          }
          if ((i2 == 1)) {
            continue;
          }
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMax(result));
      }
    }
    break;
  }
  case 1: {
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
        result = (result + WaveActiveMin(result));
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
    case 2: {
        if (true) {
          result = (result + WaveActiveSum(3));
        }
        break;
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
        if (((WaveGetLaneIndex() % 2) == 0)) {
          result = (result + WaveActiveSum(2));
        }
        break;
      }
    case 2: {
        if ((WaveGetLaneIndex() == 1)) {
          if ((WaveGetLaneIndex() == 6)) {
            result = (result + WaveActiveMin(4));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMax(3));
            }
          }
          if ((WaveGetLaneIndex() == 1)) {
            result = (result + WaveActiveMax(result));
          }
        }
        break;
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      uint counter3 = 0;
      while ((counter3 < 3)) {
        counter3 = (counter3 + 1);
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
          if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(3));
          }
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      }
      break;
    }
  case 1: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
            if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 11))) {
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
      case 2: {
          if ((WaveGetLaneIndex() == 15)) {
            if ((WaveGetLaneIndex() == 2)) {
              result = (result + WaveActiveMin(result));
            }
          } else {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
          }
        }
        break;
      }
    }
    break;
  }
  case 2: {
    for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
      for (uint i6 = 0; (i6 < 3); i6 = (i6 + 1)) {
        if ((WaveGetLaneIndex() >= 15)) {
          result = (result + WaveActiveSum(1));
        }
        if ((WaveGetLaneIndex() >= 13)) {
          result = (result + WaveActiveSum(1));
        }
      }
    }
    break;
  }
  }
}
