require 'helper'

describe 'Chipper tokens' do
  before do
    @tweet = "@youtube, why is that stories abt @apple on #cnn always get #removed from http://www1.youtube.com/videos/?"
  end

  after do
    Chipper.skip_tokens    nil
  end

  it 'should extract tokens' do
    expected = [%w(why), %w(that stories abt), %w(always get), %w(from)]
    Chipper.tokens(@tweet).must_equal expected
  end

  it 'should skip tokens' do
    expected = [%w(why), %w(that), %w(abt), %w(get)]
    Chipper.skip_tokens(%w(story always from video))
    Chipper.tokens(@tweet).must_equal expected
  end

  it 'should skip numbers dates and times' do
    expected = [["flubble","bubble"],["champion"]]
    Chipper.skip_token_pattern %q{^(?:[\d\-]+)(?:am|pm|th|st)?$}
    Chipper.tokens("flubble bubble I or 123454 123-123-1111 19 on te champion 19th at 4am or 12pm").must_equal expected
  end

  it 'should be lowercase' do
    expected = [["flubble","bubble"]]
    Chipper.tokens("FLUBBLE BUBBLE").must_equal expected
  end

  it 'stop words should not be case sensitive' do
    expected = [["pancakes"]]
    Chipper.skip_tokens(%w(christmas))
    Chipper.tokens("pancakes in Christmas").must_equal expected
  end

  it 'should filter stemmed words that are too short' do
    expected = [["flubble","bubble"]]
    Chipper.tokens("I am going to doing, being flubble bubble its").must_equal expected
  end

  # eg don't, won't aren't etc.  Quick 'cheats' way to do this is just sub out all single quotes with nothing.
  it 'should remove abbreviations with single quotes' do
    expected = [["flubble","bubble","dont"]]
    Chipper.tokens("flubble bubble don't do it").must_equal expected
  end

  it 'should segment across urls' do
    expected = [%w(hello world), %w(this), %w(might work)]
    Chipper.tokens('hello world, this http://www.example.com/1 might work').must_equal expected
  end

  describe 'segment across short, stop and non-words' do
    before do
      @expected = [%w(flopper bopper), %w(dopper)]
      Chipper.skip_tokens(%w(four five six stopper))
    end

    after do
      Chipper.skip_tokens nil
    end

    it 'should segment correctly on short word' do
      Chipper.tokens('Flopper Bopper at Dopper').must_equal @expected
    end

    it 'should segment correctly on stop word' do
      Chipper.tokens('Flopper Bopper STOPPER Dopper').must_equal @expected
    end

    it 'should segment correctly on non-word' do
      Chipper.tokens('Flopper Bopper. Dopper').must_equal @expected
    end
  end

  describe 'unicode' do
    it 'should skip quotes and handle fullwidth @' do
      text = "hello world, ain\u2019t this \uff20cool huh\u201d"
      Chipper.tokens(text).must_equal [["hello", "world"], ["aint", "this"], ["huh"]]
    end
  end
end
