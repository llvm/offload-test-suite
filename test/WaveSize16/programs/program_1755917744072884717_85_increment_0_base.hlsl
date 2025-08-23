[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() >= 12)) {
    if ((WaveGetLaneIndex() >= 15)) {
      result = (result + WaveActiveMax(result));
    }
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
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
    if ((WaveGetLaneIndex() < 3)) {
      result = (result + WaveActiveMax(4));
    }
  } else {
  if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveSum(result));
  }
  uint counter0 = 0;
  while ((counter0 < 3)) {
    counter0 = (counter0 + 1);
    uint counter1 = 0;
    while ((counter1 < 3)) {
      counter1 = (counter1 + 1);
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveSum(result));
      }
    }
  }
  if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
    result = (result + WaveActiveMax(WaveGetLaneIndex()));
  }
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveSum(1));
  } else {
  if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10))) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
  } else {
  if ((WaveGetLaneIndex() == 6)) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  } else {
  if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 12))) {
    result = (result + WaveActiveSum(4));
  }
  }
  }
  }
}
