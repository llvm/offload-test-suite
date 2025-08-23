[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveSum(result));
            }
            if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9))) {
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
      for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
          if ((WaveGetLaneIndex() >= 9)) {
            result = (result + WaveActiveMin(result));
          }
          if ((WaveGetLaneIndex() >= 14)) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
      break;
    }
  case 2: {
      uint counter3 = 0;
      while ((counter3 < 2)) {
        counter3 = (counter3 + 1);
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax(5));
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
  if ((WaveGetLaneIndex() == 1)) {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(result));
          }
          if ((WaveGetLaneIndex() == 2)) {
            if ((WaveGetLaneIndex() == 12)) {
              result = (result + WaveActiveSum(result));
            }
            if ((WaveGetLaneIndex() == 12)) {
              result = (result + WaveActiveMax(result));
            }
          }
        } else {
        if ((WaveGetLaneIndex() >= 13)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
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
        if ((WaveGetLaneIndex() < 7)) {
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
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  }
  if ((WaveGetLaneIndex() == 4)) {
    result = (result + WaveActiveSum(10));
  }
  } else {
  for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
    if ((WaveGetLaneIndex() == 5)) {
      result = (result + WaveActiveSum(result));
    }
    for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
      if ((WaveGetLaneIndex() < 1)) {
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
        }
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      } else {
      if ((WaveGetLaneIndex() >= 11)) {
        result = (result + WaveActiveSum(result));
      }
      if ((WaveGetLaneIndex() >= 14)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
      }
    }
  }
  if ((WaveGetLaneIndex() == 4)) {
    result = (result + WaveActiveMin(result));
  }
  if ((i4 == 1)) {
    continue;
  }
  if ((i4 == 2)) {
    break;
  }
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveMax(result));
  }
  }
}
