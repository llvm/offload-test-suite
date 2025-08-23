[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 0)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(result));
    }
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveSum(result));
          }
        } else {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
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
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
  } else {
  if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  } else {
  if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 12))) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
  } else {
  if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11))) {
    result = (result + WaveActiveMin(5));
  }
  }
  }
  }
  }
  if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
    if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 15))) {
      result = (result + WaveActiveMax(result));
    }
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveSum(result));
      }
      if ((WaveGetLaneIndex() < 5)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax(result));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((WaveGetLaneIndex() >= 9)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      } else {
      if ((WaveGetLaneIndex() < 6)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
      }
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax(result));
        }
      }
    }
    if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 10))) {
      result = (result + WaveActiveSum(8));
    }
    if ((i0 == 1)) {
      break;
    }
  }
  if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
  }
  }
}
