


    // !!! This file is generated using emlearn !!!

    #include <stdint.h>

// Where the model's code goes. By default these are ordinary functions. A
// firmware can define CAN_IDS_FN / CAN_IDS_PUBLIC before including this file
// to put them in a linker section, for example RAM (stm32_ids/src/classify.c).
#ifndef CAN_IDS_FN
#define CAN_IDS_FN static inline
#endif
#ifndef CAN_IDS_PUBLIC
#define CAN_IDS_PUBLIC
#endif

    

CAN_IDS_FN int32_t can_ids_tree_0(const float *features, int32_t features_length) {
          if (features[5] < 143.500000f) {
              if (features[0] < 0.500000f) {
                  if (features[2] < 2.500000f) {
                      if (features[6] < 69.500000f) {
                          return 1;
                      } else {
                          return 2;
                      }
                  } else {
                      return 2;
                  }
              } else {
                  if (features[3] < 191.500000f) {
                      if (features[4] < 154.500000f) {
                          if (features[4] < 0.500000f) {
                              if (features[3] < 141.500000f) {
                                  if (features[0] < 1709.500000f) {
                                      if (features[0] < 1682.500000f) {
                                          if (features[0] < 1521.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 0.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[6] < 1.500000f) {
                                      return 0;
                                  } else {
                                      if (features[5] < 33.500000f) {
                                          if (features[0] < 801.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 3.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[5] < 105.500000f) {
                                  if (features[3] < 40.500000f) {
                                      if (features[2] < 100.500000f) {
                                          if (features[5] < 48.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 178.500000f) {
                                          if (features[6] < 18.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 809.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 9.500000f) {
                                      if (features[0] < 1352.500000f) {
                                          if (features[5] < 140.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[2] < 0.500000f) {
                                          if (features[6] < 25.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 1087.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[5] < 105.500000f) {
                              if (features[3] < 50.500000f) {
                                  if (features[3] < 23.500000f) {
                                      if (features[3] < 0.500000f) {
                                          if (features[6] < 16.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 790.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 5.500000f) {
                                          if (features[4] < 252.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 53.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[4] < 252.500000f) {
                                      if (features[3] < 136.500000f) {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 48.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 150.500000f) {
                                          if (features[4] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 82.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[5] < 112.500000f) {
                                  if (features[6] < 15.500000f) {
                                      if (features[0] < 1223.500000f) {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[6] < 0.500000f) {
                                      if (features[2] < 157.500000f) {
                                          if (features[3] < 149.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 1173.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 128.500000f) {
                                          if (features[3] < 186.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[4] < 17.500000f) {
                          if (features[0] < 688.500000f) {
                              if (features[1] < 6.500000f) {
                                  return 0;
                              } else {
                                  if (features[0] < 496.500000f) {
                                      return 2;
                                  } else {
                                      if (features[0] < 498.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[5] < 140.500000f) {
                                  if (features[0] < 1349.500000f) {
                                      if (features[0] < 1200.500000f) {
                                          return 2;
                                      } else {
                                          if (features[3] < 224.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[5] < 142.500000f) {
                                      if (features[6] < 2.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[6] < 0.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[1] < 6.500000f) {
                              return 0;
                          } else {
                              if (features[0] < 1201.500000f) {
                                  if (features[2] < 0.500000f) {
                                      if (features[0] < 505.500000f) {
                                          if (features[3] < 239.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 1198.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 192.500000f) {
                                          if (features[5] < 20.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 1200.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[6] < 40.500000f) {
                  if (features[0] < 497.500000f) {
                      if (features[3] < 136.500000f) {
                          if (features[0] < 303.500000f) {
                              return 2;
                          } else {
                              if (features[0] < 339.500000f) {
                                  if (features[4] < 16.500000f) {
                                      if (features[5] < 243.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      if (features[0] < 304.500000f) {
                                          if (features[4] < 55.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[2] < 0.500000f) {
                                      if (features[0] < 477.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[2] < 0.500000f) {
                              if (features[0] < 474.500000f) {
                                  if (features[3] < 151.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 0;
                              }
                          } else {
                              if (features[4] < 0.500000f) {
                                  if (features[3] < 145.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  } else {
                      if (features[6] < 0.500000f) {
                          if (features[5] < 254.500000f) {
                              if (features[3] < 254.500000f) {
                                  if (features[0] < 1194.500000f) {
                                      return 2;
                                  } else {
                                      if (features[0] < 1352.500000f) {
                                          if (features[0] < 1202.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              return 2;
                          }
                      } else {
                          return 2;
                      }
                  }
              } else {
                  if (features[6] < 41.500000f) {
                      if (features[0] < 785.500000f) {
                          if (features[5] < 254.500000f) {
                              return 2;
                          } else {
                              return 0;
                          }
                      } else {
                          if (features[3] < 42.500000f) {
                              if (features[4] < 37.500000f) {
                                  if (features[4] < 35.500000f) {
                                      return 2;
                                  } else {
                                      return 4;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              return 2;
                          }
                      }
                  } else {
                      if (features[3] < 64.500000f) {
                          if (features[2] < 16.500000f) {
                              if (features[6] < 126.500000f) {
                                  if (features[5] < 254.500000f) {
                                      return 2;
                                  } else {
                                      if (features[4] < 52.500000f) {
                                          return 2;
                                      } else {
                                          if (features[4] < 100.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[3] < 0.500000f) {
                                      if (features[2] < 0.500000f) {
                                          if (features[0] < 696.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[6] < 254.500000f) {
                                  if (features[6] < 169.500000f) {
                                      return 2;
                                  } else {
                                      if (features[2] < 50.500000f) {
                                          if (features[3] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[2] < 48.500000f) {
                                      if (features[5] < 252.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 1672.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[3] < 69.500000f) {
                              if (features[2] < 1.500000f) {
                                  if (features[6] < 106.500000f) {
                                      if (features[0] < 939.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      if (features[4] < 109.500000f) {
                                          if (features[2] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 3;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[6] < 105.500000f) {
                                      if (features[3] < 68.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 1076.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[5] < 254.500000f) {
                                  return 2;
                              } else {
                                  if (features[4] < 96.500000f) {
                                      if (features[3] < 145.500000f) {
                                          if (features[4] < 8.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_1(const float *features, int32_t features_length) {
          if (features[0] < 0.500000f) {
              if (features[5] < 0.500000f) {
                  if (features[3] < 78.500000f) {
                      return 1;
                  } else {
                      return 2;
                  }
              } else {
                  return 2;
              }
          } else {
              if (features[6] < 40.500000f) {
                  if (features[0] < 1520.500000f) {
                      if (features[3] < 192.500000f) {
                          if (features[4] < 128.500000f) {
                              if (features[5] < 48.500000f) {
                                  if (features[3] < 39.500000f) {
                                      if (features[5] < 10.500000f) {
                                          if (features[2] < 253.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[3] < 24.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 881.500000f) {
                                          if (features[4] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 180.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[4] < 16.500000f) {
                                      if (features[0] < 303.500000f) {
                                          return 2;
                                      } else {
                                          if (features[5] < 124.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 0.500000f) {
                                          if (features[0] < 499.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[2] < 5.500000f) {
                                  if (features[5] < 21.500000f) {
                                      if (features[0] < 460.500000f) {
                                          return 2;
                                      } else {
                                          if (features[2] < 3.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 0.500000f) {
                                          if (features[6] < 25.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 789.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 0.500000f) {
                                      if (features[0] < 1202.500000f) {
                                          if (features[5] < 26.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[0] < 160.500000f) {
                                          if (features[3] < 122.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 790.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[6] < 0.500000f) {
                              if (features[3] < 213.500000f) {
                                  if (features[5] < 218.500000f) {
                                      if (features[4] < 202.500000f) {
                                          if (features[0] < 469.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 11.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 193.500000f) {
                                          if (features[2] < 42.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 1178.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[0] < 496.500000f) {
                                      return 2;
                                  } else {
                                      if (features[1] < 6.500000f) {
                                          return 0;
                                      } else {
                                          if (features[3] < 248.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[5] < 7.500000f) {
                                  if (features[0] < 691.500000f) {
                                      if (features[3] < 234.500000f) {
                                          if (features[2] < 4.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 238.500000f) {
                                      if (features[2] < 0.500000f) {
                                          if (features[0] < 366.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[4] < 0.500000f) {
                          if (features[5] < 1.500000f) {
                              return 0;
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[4] < 2.500000f) {
                              if (features[6] < 0.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[0] < 2008.500000f) {
                                  return 2;
                              } else {
                                  if (features[4] < 8.500000f) {
                                      if (features[2] < 19.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  }
              } else {
                  if (features[2] < 1.500000f) {
                      if (features[3] < 64.500000f) {
                          if (features[6] < 51.500000f) {
                              if (features[4] < 29.500000f) {
                                  if (features[5] < 10.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[2] < 0.500000f) {
                                  if (features[0] < 671.500000f) {
                                      if (features[6] < 230.500000f) {
                                          if (features[4] < 69.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 214.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[1] < 6.500000f) {
                                          return 0;
                                      } else {
                                          if (features[6] < 161.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[3] < 4.500000f) {
                                      if (features[4] < 17.500000f) {
                                          if (features[3] < 1.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 17.500000f) {
                                          if (features[4] < 66.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[2] < 0.500000f) {
                              if (features[5] < 0.500000f) {
                                  if (features[4] < 86.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[4] < 0.500000f) {
                                      if (features[3] < 149.500000f) {
                                          if (features[3] < 125.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 511.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 689.500000f) {
                                          if (features[3] < 239.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[3] < 69.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 106.500000f) {
                                  if (features[5] < 254.500000f) {
                                      if (features[3] < 252.500000f) {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 0;
                                  }
                              } else {
                                  if (features[5] < 253.500000f) {
                                      if (features[5] < 7.500000f) {
                                          if (features[4] < 88.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[4] < 80.500000f) {
                                          if (features[4] < 32.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 3;
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[3] < 40.500000f) {
                          if (features[6] < 104.500000f) {
                              if (features[1] < 6.500000f) {
                                  return 0;
                              } else {
                                  if (features[4] < 16.500000f) {
                                      if (features[4] < 12.500000f) {
                                          if (features[3] < 1.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 34.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 790.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[5] < 105.500000f) {
                                  if (features[6] < 254.500000f) {
                                      if (features[0] < 848.500000f) {
                                          if (features[6] < 151.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 106.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 4.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 1096.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 254.500000f) {
                                      if (features[4] < 6.500000f) {
                                          if (features[0] < 2021.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[3] < 4.500000f) {
                                          if (features[2] < 249.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 11.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[5] < 254.500000f) {
                              if (features[0] < 790.500000f) {
                                  if (features[5] < 48.500000f) {
                                      if (features[0] < 304.500000f) {
                                          if (features[6] < 78.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[4] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[6] < 254.500000f) {
                                      if (features[5] < 17.500000f) {
                                          if (features[6] < 169.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 18.500000f) {
                                          if (features[0] < 1973.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 3.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 41.500000f) {
                                  if (features[0] < 547.500000f) {
                                      return 0;
                                  } else {
                                      return 4;
                                  }
                              } else {
                                  if (features[6] < 126.500000f) {
                                      if (features[0] < 1087.500000f) {
                                          if (features[6] < 118.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[0] < 306.500000f) {
                                          if (features[3] < 146.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_2(const float *features, int32_t features_length) {
          if (features[5] < 143.500000f) {
              if (features[2] < 0.500000f) {
                  if (features[4] < 0.500000f) {
                      if (features[1] < 6.500000f) {
                          return 0;
                      } else {
                          if (features[0] < 1.500000f) {
                              return 1;
                          } else {
                              if (features[6] < 242.500000f) {
                                  if (features[6] < 26.500000f) {
                                      return 0;
                                  } else {
                                      if (features[5] < 43.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[0] < 319.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[3] < 0.500000f) {
                          if (features[1] < 6.500000f) {
                              return 0;
                          } else {
                              if (features[5] < 15.500000f) {
                                  if (features[5] < 7.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 0;
                              }
                          }
                      } else {
                          if (features[6] < 25.500000f) {
                              if (features[5] < 131.500000f) {
                                  if (features[0] < 1379.500000f) {
                                      if (features[0] < 493.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 499.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[4] < 113.500000f) {
                                      return 2;
                                  } else {
                                      if (features[6] < 3.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          } else {
                              if (features[1] < 6.500000f) {
                                  return 0;
                              } else {
                                  return 2;
                              }
                          }
                      }
                  }
              } else {
                  if (features[3] < 191.500000f) {
                      if (features[4] < 154.500000f) {
                          if (features[0] < 1520.500000f) {
                              if (features[0] < 159.500000f) {
                                  return 2;
                              } else {
                                  if (features[5] < 48.500000f) {
                                      if (features[5] < 0.500000f) {
                                          if (features[3] < 142.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 101.500000f) {
                                          if (features[4] < 114.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 847.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 0.500000f) {
                                  if (features[4] < 9.500000f) {
                                      return 0;
                                  } else {
                                      if (features[0] < 2005.500000f) {
                                          return 2;
                                      } else {
                                          if (features[4] < 31.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 254.500000f) {
                                      if (features[0] < 2024.500000f) {
                                          return 2;
                                      } else {
                                          if (features[2] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 20.500000f) {
                                          if (features[0] < 1972.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 120.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[6] < 122.500000f) {
                              if (features[3] < 50.500000f) {
                                  if (features[3] < 22.500000f) {
                                      if (features[6] < 0.500000f) {
                                          if (features[3] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 790.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 105.500000f) {
                                          if (features[5] < 7.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 1200.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 0.500000f) {
                                      if (features[0] < 1214.500000f) {
                                          if (features[2] < 126.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 217.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 30.500000f) {
                                          if (features[6] < 50.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 32.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[5] < 48.500000f) {
                                  if (features[4] < 174.500000f) {
                                      if (features[5] < 47.500000f) {
                                          if (features[0] < 790.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 618.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 155.500000f) {
                                          if (features[3] < 123.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[2] < 5.500000f) {
                                      if (features[5] < 103.500000f) {
                                          return 2;
                                      } else {
                                          if (features[2] < 4.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[1] < 6.500000f) {
                          return 0;
                      } else {
                          if (features[6] < 0.500000f) {
                              if (features[0] < 1196.500000f) {
                                  return 2;
                              } else {
                                  if (features[0] < 1349.500000f) {
                                      if (features[3] < 193.500000f) {
                                          if (features[2] < 44.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 7.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[0] < 809.500000f) {
                                  if (features[3] < 192.500000f) {
                                      if (features[6] < 17.500000f) {
                                          if (features[0] < 755.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[2] < 68.500000f) {
                  if (features[4] < 16.500000f) {
                      if (features[5] < 254.500000f) {
                          if (features[2] < 0.500000f) {
                              if (features[3] < 42.500000f) {
                                  if (features[6] < 90.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[6] < 18.500000f) {
                                      if (features[6] < 0.500000f) {
                                          return 0;
                                      } else {
                                          if (features[6] < 10.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[0] < 1200.500000f) {
                                  return 2;
                              } else {
                                  if (features[3] < 83.500000f) {
                                      if (features[3] < 1.500000f) {
                                          if (features[0] < 1939.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[3] < 58.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 0.500000f) {
                                          if (features[0] < 1284.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[4] < 0.500000f) {
                              return 0;
                          } else {
                              if (features[0] < 344.500000f) {
                                  if (features[2] < 41.500000f) {
                                      if (features[0] < 301.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[6] < 249.500000f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[2] < 0.500000f) {
                          if (features[4] < 144.500000f) {
                              if (features[6] < 127.500000f) {
                                  if (features[5] < 254.500000f) {
                                      if (features[0] < 507.500000f) {
                                          if (features[5] < 243.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[3] < 247.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 116.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[4] < 64.500000f) {
                                      if (features[3] < 1.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[6] < 16.500000f) {
                                  if (features[0] < 587.500000f) {
                                      if (features[0] < 458.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      if (features[0] < 1196.500000f) {
                                          return 2;
                                      } else {
                                          if (features[6] < 2.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[5] < 254.500000f) {
                              if (features[3] < 197.500000f) {
                                  if (features[2] < 58.500000f) {
                                      if (features[2] < 1.500000f) {
                                          if (features[3] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[4] < 91.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 123.500000f) {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[4] < 62.500000f) {
                                      if (features[3] < 224.500000f) {
                                          if (features[0] < 1195.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 1197.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 60.500000f) {
                                          if (features[6] < 0.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[3] < 199.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[4] < 96.500000f) {
                                  if (features[2] < 1.500000f) {
                                      if (features[6] < 106.500000f) {
                                          return 0;
                                      } else {
                                          return 3;
                                      }
                                  } else {
                                      if (features[6] < 128.500000f) {
                                          if (features[4] < 95.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[3] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  }
              } else {
                  if (features[0] < 790.500000f) {
                      if (features[3] < 41.500000f) {
                          if (features[6] < 41.500000f) {
                              if (features[3] < 40.500000f) {
                                  return 2;
                              } else {
                                  if (features[4] < 39.500000f) {
                                      return 4;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[4] < 0.500000f) {
                              if (features[2] < 250.500000f) {
                                  if (features[6] < 127.500000f) {
                                      if (features[3] < 146.500000f) {
                                          if (features[5] < 253.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 252.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  }
                              } else {
                                  if (features[5] < 251.500000f) {
                                      return 2;
                                  } else {
                                      return 0;
                                  }
                              }
                          } else {
                              if (features[5] < 254.500000f) {
                                  return 2;
                              } else {
                                  if (features[2] < 254.500000f) {
                                      if (features[2] < 253.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 293.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 109.500000f) {
                                          return 2;
                                      } else {
                                          if (features[6] < 144.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[6] < 0.500000f) {
                          if (features[5] < 254.500000f) {
                              if (features[3] < 254.500000f) {
                                  if (features[4] < 25.500000f) {
                                      if (features[3] < 229.500000f) {
                                          if (features[3] < 101.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 213.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[3] < 205.500000f) {
                                          if (features[0] < 1205.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 1204.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              return 2;
                          }
                      } else {
                          if (features[3] < 69.500000f) {
                              if (features[3] < 68.500000f) {
                                  return 2;
                              } else {
                                  if (features[2] < 204.500000f) {
                                      if (features[6] < 104.500000f) {
                                          if (features[6] < 90.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              return 2;
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_3(const float *features, int32_t features_length) {
          if (features[0] < 0.500000f) {
              if (features[4] < 1.500000f) {
                  if (features[6] < 77.500000f) {
                      return 1;
                  } else {
                      return 2;
                  }
              } else {
                  return 2;
              }
          } else {
              if (features[4] < 35.500000f) {
                  if (features[0] < 1680.500000f) {
                      if (features[6] < 66.500000f) {
                          if (features[5] < 0.500000f) {
                              if (features[2] < 34.500000f) {
                                  if (features[4] < 16.500000f) {
                                      return 0;
                                  } else {
                                      if (features[6] < 9.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[3] < 165.500000f) {
                                      if (features[2] < 112.500000f) {
                                          if (features[4] < 2.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[0] < 1349.500000f) {
                                  if (features[0] < 159.500000f) {
                                      return 2;
                                  } else {
                                      if (features[6] < 8.500000f) {
                                          if (features[5] < 47.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 790.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[3] < 130.500000f) {
                              if (features[6] < 253.500000f) {
                                  if (features[5] < 7.500000f) {
                                      if (features[4] < 17.500000f) {
                                          if (features[5] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 6.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 122.500000f) {
                                          if (features[2] < 31.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 304.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[5] < 49.500000f) {
                                      if (features[0] < 1146.500000f) {
                                          if (features[6] < 254.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[6] < 254.500000f) {
                                          if (features[3] < 121.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[1] < 6.500000f) {
                                  return 0;
                              } else {
                                  if (features[0] < 304.500000f) {
                                      if (features[3] < 144.500000f) {
                                          if (features[5] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 156.500000f) {
                                          if (features[5] < 31.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[1] < 6.500000f) {
                          return 0;
                      } else {
                          if (features[3] < 127.500000f) {
                              if (features[5] < 18.500000f) {
                                  if (features[2] < 4.500000f) {
                                      if (features[3] < 114.500000f) {
                                          if (features[5] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      if (features[3] < 8.500000f) {
                                          if (features[3] < 7.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[6] < 254.500000f) {
                                      if (features[3] < 0.500000f) {
                                          if (features[4] < 8.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 254.500000f) {
                                          if (features[3] < 124.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          } else {
                              return 2;
                          }
                      }
                  }
              } else {
                  if (features[4] < 36.500000f) {
                      if (features[6] < 41.500000f) {
                          if (features[5] < 254.500000f) {
                              if (features[0] < 793.500000f) {
                                  if (features[0] < 159.500000f) {
                                      return 2;
                                  } else {
                                      if (features[6] < 38.500000f) {
                                          if (features[6] < 9.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[3] < 45.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[0] < 1201.500000f) {
                                      if (features[0] < 1195.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[3] < 55.500000f) {
                                  return 4;
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[2] < 36.500000f) {
                              if (features[3] < 82.500000f) {
                                  if (features[5] < 107.500000f) {
                                      if (features[0] < 877.500000f) {
                                          if (features[0] < 605.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[6] < 112.500000f) {
                                      if (features[6] < 98.500000f) {
                                          return 2;
                                      } else {
                                          if (features[5] < 28.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[6] < 254.500000f) {
                                  if (features[2] < 53.500000f) {
                                      if (features[2] < 52.500000f) {
                                          return 2;
                                      } else {
                                          if (features[6] < 180.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 70.500000f) {
                                      if (features[3] < 15.500000f) {
                                          return 0;
                                      } else {
                                          if (features[5] < 64.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[5] < 105.500000f) {
                          if (features[0] < 1201.500000f) {
                              if (features[0] < 607.500000f) {
                                  if (features[1] < 7.500000f) {
                                      return 0;
                                  } else {
                                      if (features[0] < 160.500000f) {
                                          if (features[6] < 28.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[3] < 192.500000f) {
                                      if (features[2] < 220.500000f) {
                                          if (features[1] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[3] < 5.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 0.500000f) {
                                          if (features[4] < 137.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[2] < 5.500000f) {
                                  if (features[6] < 0.500000f) {
                                      if (features[4] < 178.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[2] < 2.500000f) {
                                          return 2;
                                      } else {
                                          if (features[4] < 84.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[0] < 1086.500000f) {
                              if (features[2] < 0.500000f) {
                                  if (features[5] < 241.500000f) {
                                      if (features[3] < 239.500000f) {
                                          if (features[0] < 499.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[5] < 254.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 364.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[2] < 0.500000f) {
                                  if (features[0] < 1088.500000f) {
                                      return 0;
                                  } else {
                                      if (features[4] < 201.500000f) {
                                          if (features[6] < 1.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 107.500000f) {
                                      if (features[6] < 106.500000f) {
                                          if (features[0] < 1201.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[5] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 3;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 1087.500000f) {
                                          if (features[4] < 97.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 3.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_FN int32_t can_ids_tree_4(const float *features, int32_t features_length) {
          if (features[5] < 143.500000f) {
              if (features[1] < 7.500000f) {
                  return 0;
              } else {
                  if (features[0] < 0.500000f) {
                      if (features[2] < 0.500000f) {
                          return 1;
                      } else {
                          return 2;
                      }
                  } else {
                      if (features[3] < 191.500000f) {
                          if (features[0] < 1680.500000f) {
                              if (features[5] < 48.500000f) {
                                  if (features[4] < 154.500000f) {
                                      if (features[0] < 1201.500000f) {
                                          if (features[5] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 1679.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[2] < 5.500000f) {
                                          if (features[0] < 483.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[2] < 115.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[6] < 122.500000f) {
                                      if (features[3] < 105.500000f) {
                                          if (features[5] < 101.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 136.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 254.500000f) {
                                          if (features[6] < 123.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 254.500000f) {
                                  if (features[2] < 3.500000f) {
                                      if (features[3] < 0.500000f) {
                                          if (features[6] < 24.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 2008.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 0.500000f) {
                                          if (features[4] < 7.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[0] < 1988.500000f) {
                                      if (features[2] < 14.500000f) {
                                          if (features[5] < 117.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[2] < 17.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[2] < 0.500000f) {
                              if (features[6] < 26.500000f) {
                                  if (features[6] < 1.500000f) {
                                      if (features[3] < 206.500000f) {
                                          if (features[3] < 198.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[0] < 756.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[5] < 61.500000f) {
                                          if (features[6] < 18.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[0] < 356.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[4] < 0.500000f) {
                                  if (features[6] < 0.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 192.500000f) {
                                      if (features[5] < 20.500000f) {
                                          if (features[6] < 18.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[6] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[0] < 1200.500000f) {
                                          return 2;
                                      } else {
                                          if (features[0] < 1201.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  }
              }
          } else {
              if (features[2] < 68.500000f) {
                  if (features[4] < 16.500000f) {
                      if (features[0] < 497.500000f) {
                          if (features[0] < 303.500000f) {
                              return 2;
                          } else {
                              if (features[5] < 254.500000f) {
                                  if (features[5] < 241.500000f) {
                                      if (features[6] < 18.500000f) {
                                          if (features[5] < 161.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[6] < 141.500000f) {
                                      return 0;
                                  } else {
                                      if (features[6] < 142.500000f) {
                                          if (features[4] < 0.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          return 0;
                                      }
                                  }
                              }
                          }
                      } else {
                          if (features[6] < 0.500000f) {
                              if (features[2] < 31.500000f) {
                                  if (features[2] < 16.500000f) {
                                      if (features[0] < 1234.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  } else {
                                      if (features[3] < 94.500000f) {
                                          if (features[0] < 1492.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      } else {
                                          if (features[2] < 23.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              } else {
                                  if (features[0] < 1407.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          } else {
                              if (features[0] < 2023.500000f) {
                                  return 2;
                              } else {
                                  if (features[3] < 4.500000f) {
                                      return 0;
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  } else {
                      if (features[6] < 111.500000f) {
                          if (features[2] < 1.500000f) {
                              if (features[2] < 0.500000f) {
                                  if (features[6] < 24.500000f) {
                                      if (features[5] < 242.500000f) {
                                          if (features[4] < 225.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[6] < 6.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 96.500000f) {
                                          if (features[5] < 253.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  if (features[3] < 69.500000f) {
                                      if (features[6] < 106.500000f) {
                                          if (features[3] < 67.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 199.500000f) {
                                              return 2;
                                          } else {
                                              return 3;
                                          }
                                      }
                                  } else {
                                      if (features[6] < 0.500000f) {
                                          return 0;
                                      } else {
                                          if (features[5] < 254.500000f) {
                                              return 2;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  }
                              }
                          } else {
                              if (features[6] < 89.500000f) {
                                  if (features[6] < 0.500000f) {
                                      if (features[0] < 1187.500000f) {
                                          return 2;
                                      } else {
                                          if (features[4] < 93.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[3] < 71.500000f) {
                                      if (features[3] < 68.500000f) {
                                          return 2;
                                      } else {
                                          if (features[3] < 69.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      } else {
                          if (features[5] < 254.500000f) {
                              return 2;
                          } else {
                              if (features[0] < 1086.500000f) {
                                  if (features[3] < 2.500000f) {
                                      if (features[6] < 165.500000f) {
                                          return 2;
                                      } else {
                                          return 0;
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[4] < 96.500000f) {
                                      if (features[0] < 1125.500000f) {
                                          return 0;
                                      } else {
                                          if (features[6] < 252.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              }
                          }
                      }
                  }
              } else {
                  if (features[5] < 254.500000f) {
                      if (features[6] < 0.500000f) {
                          if (features[0] < 1194.500000f) {
                              return 2;
                          } else {
                              if (features[5] < 165.500000f) {
                                  if (features[0] < 1360.500000f) {
                                      if (features[5] < 150.500000f) {
                                          return 0;
                                      } else {
                                          if (features[0] < 1207.500000f) {
                                              return 0;
                                          } else {
                                              return 2;
                                          }
                                      }
                                  } else {
                                      return 2;
                                  }
                              } else {
                                  if (features[2] < 103.500000f) {
                                      if (features[5] < 244.500000f) {
                                          if (features[3] < 51.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[4] < 235.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  } else {
                                      if (features[4] < 40.500000f) {
                                          if (features[2] < 149.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          if (features[5] < 237.500000f) {
                                              return 0;
                                          } else {
                                              return 0;
                                          }
                                      }
                                  }
                              }
                          }
                      } else {
                          return 2;
                      }
                  } else {
                      if (features[3] < 41.500000f) {
                          if (features[3] < 40.500000f) {
                              return 2;
                          } else {
                              if (features[4] < 58.500000f) {
                                  return 4;
                              } else {
                                  return 2;
                              }
                          }
                      } else {
                          if (features[2] < 244.500000f) {
                              if (features[0] < 1087.500000f) {
                                  if (features[0] < 303.500000f) {
                                      return 2;
                                  } else {
                                      if (features[3] < 144.500000f) {
                                          if (features[3] < 68.500000f) {
                                              return 2;
                                          } else {
                                              return 0;
                                          }
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          } else {
                              if (features[4] < 8.500000f) {
                                  if (features[4] < 0.500000f) {
                                      return 0;
                                  } else {
                                      if (features[3] < 166.500000f) {
                                          return 0;
                                      } else {
                                          return 2;
                                      }
                                  }
                              } else {
                                  return 2;
                              }
                          }
                      }
                  }
              }
          }
        }
        

CAN_IDS_PUBLIC int32_t can_ids_predict(const float *features, int32_t features_length) {

        int32_t votes[5] = {0,};
        int32_t _class = -1;

        _class = can_ids_tree_0(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_1(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_2(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_3(features, features_length); votes[_class] += 1;
    _class = can_ids_tree_4(features, features_length); votes[_class] += 1;
    
        int32_t most_voted_class = -1;
        int32_t most_voted_votes = 0;
        for (int32_t i=0; i<5; i++) {

            if (votes[i] > most_voted_votes) {
                most_voted_class = i;
                most_voted_votes = votes[i];
            }
        }
        return most_voted_class;
    }
    

int can_ids_predict_proba(const float *features, int32_t features_length, float *out, int out_length) {

        int32_t _class = -1;

        for (int i=0; i<out_length; i++) {
            out[i] = 0.0f;
        }

        _class = can_ids_tree_0(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_1(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_2(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_3(features, features_length); out[_class] += 1.0f;
    _class = can_ids_tree_4(features, features_length); out[_class] += 1.0f;
    
        // compute mean
        for (int i=0; i<out_length; i++) {
            out[i] = out[i] / 5;
        }
        return 0;
    }
    
// Which model this is: shown by the firmware in READY and #STAT.
#define CAN_IDS_MODEL_ID "rf5-d9-p5-2ddda531"

// CAN IDs in the normal training traffic (38 of 2048), one bit per ID.
static const uint8_t can_ids_known[256] = {
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x46, 0x00, 0x03, 0x00, 0x00
};

CAN_IDS_FN int can_ids_id_known(uint32_t can_id)
{
    return (can_ids_known[(can_id >> 3) & 0xFF] >> (can_id & 7)) & 1;
}
