[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10))) {
            if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 11))) {
              result = (result + WaveActiveMax(7));
            }
            if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
            }
          } else {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
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
    if (((WaveGetLaneIndex() % 2) == 0)) {
      result = (result + WaveActiveSum(2));
    }
    break;
  }
  case 2: {
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 14))) {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(result));
        }
        if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
        }
      } else {
      if ((WaveGetLaneIndex() == 0)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if ((WaveGetLaneIndex() == 8)) {
        result = (result + WaveActiveMin(result));
      }
    }
    if ((i0 == 1)) {
      break;
    }
  }
  break;
  }
  }
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
      result = (result + WaveActiveSum(result));
    }
    for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
      if ((WaveGetLaneIndex() < 3)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(10));
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        } else {
        if ((WaveGetLaneIndex() == 5)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((i2 == 2)) {
        break;
      }
    }
    if ((WaveGetLaneIndex() >= 9)) {
      result = (result + WaveActiveMax(result));
    }
  }
  } else {
  if ((WaveGetLaneIndex() >= 13)) {
    result = (result + WaveActiveSum(10));
  }
  if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
      }
      if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 13))) {
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        }
      } else {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
      }
    }
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMax(result));
    }
  }
  if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
    result = (result + WaveActiveMin(result));
  }
  }
  if ((WaveGetLaneIndex() < 4)) {
    result = (result + WaveActiveSum(result));
  }
  }
  if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9))) {
    if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
    }
    for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
      if ((WaveGetLaneIndex() < 2)) {
        result = (result + WaveActiveMin(result));
      }
      for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
        if ((WaveGetLaneIndex() == 9)) {
          result = (result + WaveActiveMin(result));
        }
        if ((i5 == 1)) {
          continue;
        }
      }
      if ((WaveGetLaneIndex() >= 12)) {
        result = (result + WaveActiveSum(result));
      }
    }
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
  } else {
  for (uint i6 = 0; (i6 < 3); i6 = (i6 + 1)) {
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 11))) {
      result = (result + WaveActiveMax(result));
    }
    uint counter7 = 0;
    while ((counter7 < 3)) {
      counter7 = (counter7 + 1);
      if ((WaveGetLaneIndex() == 15)) {
        result = (result + WaveActiveMin(result));
      }
      if ((counter7 == 2)) {
        break;
      }
    }
  }
  }
}
