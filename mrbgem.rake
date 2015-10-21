MRuby::Gem::Specification.new('mruby-fm3i2cmaster') do |spec|
  spec.license = ''
  spec.author  = 'lsi-dev'
  spec.summary = 'fm3 i2c(master) class'

  spec.cc.include_paths << "#{build.root}/src"
end
