[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() == 4)) {
    if ((WaveGetLaneIndex() == 10)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
  } else {
  if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveMax(result));
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        if ((WaveGetLaneIndex() < 6)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
    }
  case 2: {
      for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveSum(result));
        }
        if ((i1 == 1)) {
          continue;
        }
      }
      break;
    }
  case 3: {
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMin(result));
        }
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(result));
        }
      }
      break;
    }
  }
  if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveSum(result));
  }
  }
}
