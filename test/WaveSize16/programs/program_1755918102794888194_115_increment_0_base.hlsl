[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() == 12)) {
        for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveSum(result));
          }
        }
      } else {
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMax(result));
      }
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 5))) {
        result = (result + WaveActiveSum(result));
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
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
        if ((WaveGetLaneIndex() == 9)) {
          result = (result + WaveActiveSum(result));
        }
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 10))) {
            result = (result + WaveActiveMin(result));
          }
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveMin(result));
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveSum(result));
          }
        }
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveSum(result));
        }
      }
    }
    break;
  }
  case 1: {
    switch ((WaveGetLaneIndex() % 4)) {
    case 0: {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(1));
        }
        break;
      }
    case 1: {
        uint counter3 = 0;
        while ((counter3 < 3)) {
          counter3 = (counter3 + 1);
          if ((WaveGetLaneIndex() < 3)) {
            result = (result + WaveActiveMax(result));
          }
          uint counter4 = 0;
          while ((counter4 < 3)) {
            counter4 = (counter4 + 1);
            if ((WaveGetLaneIndex() == 13)) {
              result = (result + WaveActiveSum(3));
            }
          }
        }
        break;
      }
    case 2: {
        for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(result));
          }
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
            if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 6))) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
            if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if ((WaveGetLaneIndex() >= 11)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
      }
      break;
    }
  case 3: {
      uint counter6 = 0;
      while ((counter6 < 3)) {
        counter6 = (counter6 + 1);
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveMax(result));
        }
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13))) {
          if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMin(result));
          }
        } else {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMax(result));
        }
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((WaveGetLaneIndex() >= 12)) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  }
  break;
  }
  case 2: {
    if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 14))) {
      for (uint i7 = 0; (i7 < 3); i7 = (i7 + 1)) {
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() < 6)) {
          if ((WaveGetLaneIndex() < 4)) {
            result = (result + WaveActiveSum(result));
          }
        } else {
        if ((WaveGetLaneIndex() == 3)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMax(result));
      }
      if ((i7 == 2)) {
        break;
      }
    }
    if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 0))) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
  } else {
  if ((WaveGetLaneIndex() < 8)) {
    result = (result + WaveActiveMax(WaveGetLaneIndex()));
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() >= 12)) {
        if ((WaveGetLaneIndex() >= 10)) {
          result = (result + WaveActiveSum(result));
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
  if ((WaveGetLaneIndex() < 6)) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
  }
  }
  break;
  }
  }
  if ((WaveGetLaneIndex() == 0)) {
    for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
      if ((WaveGetLaneIndex() == 0)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if ((WaveGetLaneIndex() == 7)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
      }
    }
    if ((WaveGetLaneIndex() == 4)) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
  }
  if ((WaveGetLaneIndex() == 4)) {
    result = (result + WaveActiveSum(result));
  }
  }
}
