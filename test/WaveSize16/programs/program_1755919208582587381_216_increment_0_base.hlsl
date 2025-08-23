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
  if (((WaveGetLaneIndex() & 1) == 0)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax(result));
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
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
  }
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    if ((WaveGetLaneIndex() == 0)) {
      result = (result + WaveActiveSum(result));
    }
    uint counter1 = 0;
    while ((counter1 < 2)) {
      counter1 = (counter1 + 1);
      if ((WaveGetLaneIndex() >= 11)) {
        result = (result + WaveActiveMin(3));
      }
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
            if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveMax(result));
            }
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
            if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
          break;
        }
      }
      if ((WaveGetLaneIndex() >= 9)) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
    }
  }
}
