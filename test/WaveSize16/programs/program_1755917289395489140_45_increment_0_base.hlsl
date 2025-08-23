[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
    if ((WaveGetLaneIndex() == 1)) {
      if ((WaveGetLaneIndex() == 5)) {
        result = (result + WaveActiveMax(result));
      }
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(9));
        }
      }
    }
    if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11))) {
      result = (result + WaveActiveMax(result));
    }
  } else {
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveSum(8));
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
    }
  case 1: {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 2: {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(result));
        }
      } else {
      if ((WaveGetLaneIndex() < 3)) {
        result = (result + WaveActiveSum(result));
      }
    }
  }
  case 3: {
    for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
      }
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
      }
      if ((i1 == 1)) {
        continue;
      }
    }
    break;
  }
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveMin(result));
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() == 0)) {
        if ((WaveGetLaneIndex() == 14)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
        for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
          }
        }
        if ((WaveGetLaneIndex() == 3)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if ((WaveGetLaneIndex() == 11)) {
        result = (result + WaveActiveMax(result));
      }
      for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
        if ((WaveGetLaneIndex() == 0)) {
          result = (result + WaveActiveSum(1));
        }
        uint counter4 = 0;
        while ((counter4 < 2)) {
          counter4 = (counter4 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
        }
      }
      if ((WaveGetLaneIndex() == 5)) {
        result = (result + WaveActiveSum(result));
      }
    }
  }
  case 1: {
    if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
      if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 7))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        uint counter5 = 0;
        while ((counter5 < 3)) {
          counter5 = (counter5 + 1);
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveMax(result));
          }
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax(result));
          }
        }
      }
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveSum(result));
      }
    } else {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMax(6));
      }
      for (uint i6 = 0; (i6 < 3); i6 = (i6 + 1)) {
        if ((WaveGetLaneIndex() == 1)) {
          result = (result + WaveActiveMin(result));
        }
        if ((WaveGetLaneIndex() == 10)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 8))) {
      result = (result + WaveActiveMin(result));
    }
  }
  }
  case 2: {
    for (uint i7 = 0; (i7 < 3); i7 = (i7 + 1)) {
      for (uint i8 = 0; (i8 < 2); i8 = (i8 + 1)) {
        if ((WaveGetLaneIndex() >= 11)) {
          result = (result + WaveActiveMin(result));
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
        if ((WaveGetLaneIndex() >= 8)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      if ((WaveGetLaneIndex() == 4)) {
        result = (result + WaveActiveSum(result));
      }
      if ((i7 == 1)) {
        continue;
      }
      if ((i7 == 2)) {
        break;
      }
    }
    break;
  }
  }
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
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 0))) {
            if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 4))) {
              result = (result + WaveActiveMax(result));
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
}
