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
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum(result));
        }
        for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
            if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
            }
            if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveMax(result));
            }
          }
          if ((i0 == 1)) {
            continue;
          }
          if ((i0 == 1)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      } else {
      if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(7));
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
        default: {
            result = (result + WaveActiveSum(99));
            break;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMin(result));
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
  case 3: {
    if ((WaveGetLaneIndex() < 20)) {
      result = (result + WaveActiveSum(4));
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
  if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
    result = (result + WaveActiveMin(2));
  } else {
  if ((WaveGetLaneIndex() < 7)) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  } else {
  if ((WaveGetLaneIndex() == 8)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
  } else {
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
  }
  }
  }
  }
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
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveSum(result));
        }
        for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
          if ((WaveGetLaneIndex() == 7)) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() == 7)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      break;
    }
  }
}
