#ifndef CPUCORES_HH
#define CPUCORES_HH

// This computer's core counts (#436). No wx, no extra libraries.
namespace CpuCores
{
  // Hardware threads (what the OS schedules on); at least 1.
  unsigned logical();

  // Physical cores; falls back to logical() where the OS will not say.
  unsigned physical();
}

#endif
