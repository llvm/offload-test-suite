[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() == 12)) {
        if ((WaveGetLaneIndex() == 10)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
        }
        if ((WaveGetLaneIndex() >= 8)) {
          uint counter0 = 0;
          while ((counter0 < 2)) {
            counter0 = (counter0 + 1);
            if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
              result = (result + WaveActiveSum(result));
            }
          }
        }
        if ((WaveGetLaneIndex() == 4)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if ((WaveGetLaneIndex() == 6)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 8))) {
          if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 2))) {
            result = (result + WaveActiveSum(9));
          }
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 7))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
          }
        }
      }
      if ((WaveGetLaneIndex() == 15)) {
        result = (result + WaveActiveMin(2));
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
        if (((WaveGetLaneIndex() % 2) == 0)) {
          result = (result + WaveActiveSum(2));
        }
        break;
      }
    case 2: {
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() == 14)) {
            if ((WaveGetLaneIndex() == 9)) {
              result = (result + WaveActiveMax(result));
            }
          } else {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax(5));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(4));
        }
        if ((i1 == 1)) {
          continue;
        }
      }
      break;
    }
  }
  break;
  }
  }
  if ((WaveGetLaneIndex() == 12)) {
    uint counter2 = 0;
    while ((counter2 < 2)) {
      counter2 = (counter2 + 1);
      for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(8));
        }
        if ((i3 == 1)) {
          continue;
        }
        if ((i3 == 2)) {
          break;
        }
      }
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveSum(result));
      }
      if ((counter2 == 1)) {
        break;
      }
    }
    if ((WaveGetLaneIndex() == 14)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
    }
  } else {
  if ((WaveGetLaneIndex() == 9)) {
    result = (result + WaveActiveMin(2));
  }
  uint counter4 = 0;
  while ((counter4 < 3)) {
    counter4 = (counter4 + 1);
    if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
    uint counter5 = 0;
    while ((counter5 < 2)) {
      counter5 = (counter5 + 1);
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 9))) {
        result = (result + WaveActiveMax(3));
      }
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    }
    if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveMin(result));
    }
  }
  if ((WaveGetLaneIndex() == 12)) {
    result = (result + WaveActiveMin(WaveGetLaneIndex()));
  }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      for (uint i6 = 0; (i6 < 2); i6 = (i6 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(8));
        }
        if ((WaveGetLaneIndex() >= 11)) {
          if ((WaveGetLaneIndex() < 3)) {
            result = (result + WaveActiveMax(result));
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveMax(result));
            }
          }
        } else {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveSum(result));
        }
      }
    }
    break;
  }
  case 1: {
    for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
      if ((WaveGetLaneIndex() == 9)) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMin(5));
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
            break;
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
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
        }
      } else {
      for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMin(result));
        }
      }
    }
    if ((WaveGetLaneIndex() == 7)) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
    }
    if ((i7 == 1)) {
      break;
    }
  }
  break;
  }
  }
}
