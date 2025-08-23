[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    if ((WaveGetLaneIndex() == 9)) {
      if ((WaveGetLaneIndex() == 13)) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 0))) {
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMin(7));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
        }
      }
    } else {
    for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin(result));
        }
      }
      if ((WaveGetLaneIndex() == 7)) {
        result = (result + WaveActiveMin(result));
      }
    }
  }
  if ((WaveGetLaneIndex() == 12)) {
    result = (result + WaveActiveSum(result));
  }
  if ((counter0 == 1)) {
    break;
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
          uint counter3 = 0;
          while ((counter3 < 3)) {
            counter3 = (counter3 + 1);
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
            }
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveSum(result));
            }
          }
          break;
        }
      }
      break;
    }
  case 2: {
      uint counter4 = 0;
      while ((counter4 < 3)) {
        counter4 = (counter4 + 1);
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
        }
        for (uint i5 = 0; (i5 < 2); i5 = (i5 + 1)) {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 2))) {
          result = (result + WaveActiveSum(result));
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
}
