/*
 * Copyright (C) 2012 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.googlecode.tesseract;

import java.util.ArrayList;
import java.util.List;

import com.googlecode.tesseract.TessBaseAPI.PageIteratorLevel;

/**
 * Java interface for the ResultIterator. Does not implement all available JNI
 * methods, but does implement enough to be useful. Comments are adapted from
 * original Tesseract source.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class ResultIterator extends PageIterator {
    static {
        System.loadLibrary("lept");
        System.loadLibrary("tess");
    }

    /** Pointer to native result iterator. */
    private final long mNativeResultIterator;

    /* package */ResultIterator(long nativeResultIterator) {
        super(nativeResultIterator);

        mNativeResultIterator = nativeResultIterator;
    }

    /**
     * Returns the text string for the current object at the given level.
     *
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @return the text string for the current object at the given level.
     */
    public String getUTF8Text(int level) {
        return nativeGetUTF8Text(mNativeResultIterator, level);
    }

    /**
     * Returns the mean confidence of the current object at the given level. The
     * number should be interpreted as a percent probability (0-100).
     *
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @return the mean confidence of the current object at the given level.
     */
    public float confidence(int level) {
        return nativeConfidence(mNativeResultIterator, level);
    }

    /**
     * Returns all possible matching text strings and their confidence level
     * for the current object at the given level.
     * <p>
     * The default matching text is blank ("").
     * The default confidence level is zero (0.0)
     *
     * @param level the page iterator level. See {@link PageIteratorLevel}.
     * @return A list of pairs with the UTF string and the confidence
     * @throws NumberFormatException If some confidence level is not a valid double
     */
    public List<Pair<String, Double>> getChoicesAndConfidence(int level) throws NumberFormatException {
        // Get the native choices
        String[] nativeChoices = nativeGetChoices(mNativeResultIterator, level);

        // Create the output list
        ArrayList<Pair<String, Double>> pairedResults = new ArrayList<Pair<String, Double>>();

        for (int i = 0; i < nativeChoices.length; i++ ) {
            // The string and the confidence level are separated by a '|'
            int separatorPosition = nativeChoices[i].lastIndexOf('|');

            // Create a pair with the choices
            String utfString = "";
            Double confidenceLevel = Double.valueOf(0);
            if (separatorPosition > 0) {
                // If the string contains a '|' separate the UTF string and the confidence level
                utfString = nativeChoices[i].substring(0, separatorPosition);
                confidenceLevel = Double.parseDouble(nativeChoices[i].substring(separatorPosition + 1));
            } else {
                // If the string contains no '|' then save the full native result as the utfString
                utfString = nativeChoices[i];
            }

            // Add the UTF string to the results
            pairedResults.add(new Pair<String, Double> (utfString, confidenceLevel));
        }

        return pairedResults;
    }

    private static native String[] nativeGetChoices(long nativeResultIterator, int level);

    private static native String nativeGetUTF8Text(long nativeResultIterator, int level);
    private static native float nativeConfidence(long nativeResultIterator, int level);

    /**
     * Container to ease passing around a tuple of two objects. This object provides a sensible
     * implementation of equals(), returning true if equals() is true on each of the contained
     * objects.
     */
    public static class Pair<F, S> {
        public final F first;
        public final S second;

        /**
         * Constructor for a Pair.
         *
         * @param first  the first object in the Pair
         * @param second the second object in the pair
         */
        public Pair(F first, S second) {
            this.first = first;
            this.second = second;
        }

        /**
         * Checks the two objects for equality by delegating to their respective
         * {@link Object#equals(Object)} methods.
         *
         * @param o the {@link Pair} to which this one is to be checked for equality
         * @return true if the underlying objects of the Pair are both considered
         * equal
         */
        @Override
        public boolean equals(Object o) {
            if (!(o instanceof Pair)) {
                return false;
            }
            Pair<?, ?> p = (Pair<?, ?>) o;
            return equal(p.first, first) && equal(p.second, second);
        }

        /**
         * Returns true if two possibly-null objects are equal.
         */
        private static boolean equal(Object a, Object b) {
            return a == b || (a != null && a.equals(b));
        }

        /**
         * Compute a hash code using the hash codes of the underlying objects
         *
         * @return a hashcode of the Pair
         */
        @Override
        public int hashCode() {
            return (first == null ? 0 : first.hashCode()) ^ (second == null ? 0 : second.hashCode());
        }

        /**
         * Convenience method for creating an appropriately typed pair.
         *
         * @param a the first object in the Pair
         * @param b the second object in the pair
         * @return a Pair that is templatized with the types of a and b
         */
        public static <A, B> Pair<A, B> create(A a, B b) {
            return new Pair<A, B>(a, b);
        }
    }
}
