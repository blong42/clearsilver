require 'hdf'

module Clearsilver

  class Hdf < Neo::Hdf
    
    def initialize
      super
      @level=0
      @cpos=""
      @position=[]
    end

    attr_reader :position, :cpos

    def push name
      @position.push(@cpos)
      @cpos+=name+"."

      if block_given?
	yield
	@cpos=@position.pop
      end
    end

    def pop
      @cpos=@position.pop
    end

    def put name, value
      value = value.to_s unless value.is_a?(String)
      self.set_value(@cpos+(name.to_s),value)
    end

    def get name
      self.get_value(@cpos+name)
    end

  end

  class Cs < Neo::Cs

  end
end

      
