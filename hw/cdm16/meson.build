cdm16_ss = ss.source_set()

cdm16_ss.add(
  when: 'CONFIG_CDM16_MACHINE',
  if_true: files('cdm16.c')
)

hw_arch += {
  'cdm16': cdm16_ss
}