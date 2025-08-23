[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 4)));
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 1: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      break;
    }
  case 2: {
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      }
      break;
    }
  }
  for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
    if ((WaveGetLaneIndex() == 9)) {
      result = (result + WaveActiveMax(4));
    }
    if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 15))) {
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax(result));
      }
      for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if ((WaveGetLaneIndex() == 3)) {
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
          }
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveMax(result));
          }
        } else {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveSum(result));
    }
  }
  if ((WaveGetLaneIndex() == 15)) {
    result = (result + WaveActiveMax(WaveGetLaneIndex()));
  }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
    }
  case 1: {
      for (uint i5 = 0; (i5 < 2); i5 = (i5 + 1)) {
        if ((WaveGetLaneIndex() < 6)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
}
