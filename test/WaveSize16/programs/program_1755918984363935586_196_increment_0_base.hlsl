[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 10))) {
    if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 9))) {
      result = (result + WaveActiveMin(4));
    }
    uint counter0 = 0;
    while ((counter0 < 2)) {
      counter0 = (counter0 + 1);
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMin(result));
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
        default: {
            result = (result + WaveActiveSum(99));
            break;
          }
        }
        if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMax(result));
        }
        if ((i1 == 1)) {
          continue;
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMax(1));
      }
    }
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax(result));
    }
  } else {
  if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 9))) {
    result = (result + WaveActiveMax(result));
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMin(result));
    }
    uint counter2 = 0;
    while ((counter2 < 3)) {
      counter2 = (counter2 + 1);
      if ((WaveGetLaneIndex() >= 10)) {
        result = (result + WaveActiveMin(result));
      }
      if ((counter2 == 2)) {
        break;
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
  }
  if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 12))) {
    result = (result + WaveActiveSum(result));
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 8))) {
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() < 7)) {
          if ((WaveGetLaneIndex() >= 13)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveSum(9));
        }
      } else {
      if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMin(5));
      }
      if ((WaveGetLaneIndex() == 15)) {
        if ((WaveGetLaneIndex() == 2)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 14))) {
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
}
