require 'yaml'
load 'font.dd'

# load yaml file
metrics = {}
File.open('FreeSans.yaml', 'r') do |f| 
  metrics = YAML.load(f)
end



BBQ.data do

  glyph_metrics_array = metrics["glyph_metrics"].map do |item|
    # convert from strings as keys to symbols
    hash = {}
    item.each {|k,v| hash[k.to_sym] = v}
    GlyphMetrics.build(hash)
  end

  if metrics["kerning"]
    glyph_kerning_array = metrics["kerning"].map do |item|
      hash = {}
      item.each {|k,v| hash[k.to_sym] = v}
      GlyphKerning.build(hash)
    end
  else
    glyph_kerning_array = []
  end

  Font.build(:texture_width => metrics["texture_width"],
             :texture_filename => "FreeSans.raw",
             :glyph_metrics_array => glyph_metrics_array,
             :glyph_kerning_array => glyph_kerning_array)
end

