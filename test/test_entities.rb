require 'helper'

describe 'Chipper entities' do
  before do
    @tweet = "@youtube, why is that stories abt @apple on #cnn always get #removed from http://t.co/IsSh1t12"
  end

  after do
    Chipper.skip_users    nil
    Chipper.skip_hashtags nil
  end

  it 'should extract users' do
    Chipper.users(@tweet).must_equal %w(@youtube @apple)
  end

  it 'should extract hashtags' do
    Chipper.hashtags(@tweet).must_equal %w(#cnn #removed)
  end

  it 'should extract urls' do
    Chipper.urls(@tweet).must_equal %w(http://t.co/IsSh1t12)
  end

  # NB, these should be only [a-zA-Z0-9]+ in the path in fact, not /w as that will include unicode chars
  it 'should extract t.co urls cleanly w/quote' do
    text = "Hello http://t.co/97CLxVkD\" http://t.co/12345678, http://t.co/xxxxxxxx. http://t.co/xxxx1234' damn!"
    urls = %w(http://t.co/97CLxVkD http://t.co/12345678 http://t.co/xxxxxxxx http://t.co/xxxx1234)
    Chipper.urls(text).must_equal(urls)
  end

  it 'should skip users' do
    Chipper.skip_users(%w(youtube))
    Chipper.users(@tweet).must_equal %w(@apple)
  end

  it 'should skip hashtags' do
    Chipper.skip_hashtags(%w(cnn))
    Chipper.hashtags(@tweet).must_equal %w(#removed)
  end

  it 'should return all entities using #entities method' do
    expected = {}
    expected.merge! users:    %w(@youtube @apple)
    expected.merge! hashtags: %w(#cnn #removed)
    expected.merge! urls:     %w(http://t.co/IsSh1t12)
    expected.merge! tokens:   [["why"], ["that", "stories", "abt"], ["always", "get"], ["from"]]

    Chipper.entities(@tweet).must_equal expected
  end
end
