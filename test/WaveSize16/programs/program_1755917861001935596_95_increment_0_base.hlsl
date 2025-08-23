[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if ((WaveGetLaneIndex() >= 9)) {
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
          }
          if ((WaveGetLaneIndex() < 3)) {
            result = (result + WaveActiveMin(10));
          }
        }
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
      }
    }
  case 1: {
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveMax(result));
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
            if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMax(result));
            }
          }
        }
      }
    }
  case 2: {
      uint counter4 = 0;
      while ((counter4 < 2)) {
        counter4 = (counter4 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
        for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
          if ((WaveGetLaneIndex() == 1)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
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
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveMin(result));
          }
          if ((i5 == 1)) {
            continue;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(4));
        }
      }
      break;
    }
  }
  if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
    if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 10))) {
      result = (result + WaveActiveSum(result));
    }
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        for (uint i6 = 0; (i6 < 2); i6 = (i6 + 1)) {
          if ((WaveGetLaneIndex() == 13)) {
            if ((WaveGetLaneIndex() == 2)) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
            }
          }
          if ((WaveGetLaneIndex() >= 10)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if ((i6 == 1)) {
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
    default: {
        result = (result + WaveActiveSum(99));
        break;
      }
    }
    if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveSum(result));
    }
  } else {
  if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveMin(WaveGetLaneIndex()));
  }
  if ((WaveGetLaneIndex() == 7)) {
    if ((WaveGetLaneIndex() == 12)) {
      result = (result + WaveActiveSum(8));
    }
    uint counter7 = 0;
    while ((counter7 < 3)) {
      counter7 = (counter7 + 1);
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if ((WaveGetLaneIndex() < 5)) {
        if ((WaveGetLaneIndex() >= 12)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
    }
    if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveMin(result));
    }
    if ((counter7 == 2)) {
      break;
    }
  }
  if ((WaveGetLaneIndex() == 10)) {
    result = (result + WaveActiveMin(8));
  }
  }
  if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 8))) {
    result = (result + WaveActiveSum(result));
  }
  }
}
